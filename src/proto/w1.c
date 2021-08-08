/*
 * Basic routines to bit-bang standard 1-Wire via a GPIO pin
 *
 * Copyright 2018 Jonathan McDowell <noodles@earth.li>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <chopstx.h>

#include "dwt.h"
#include "gpio.h"
#include "intr.h"
#include "w1.h"

#define ATOMIC_BLOCK(s)

static uint8_t w1_gpio;

uint8_t w1_crc(uint8_t *buf, uint8_t len)
{
	uint8_t i, j, crc;

	crc = 0;
	for (i = 0; i < len; i++)
	{
		crc ^= buf[i];
		for (j = 0; j < 8; j++)
		{
			crc = crc >> 1 ^ ((crc & 1) ? 0x8c : 0);
		}
	}

	return crc;
}

/**
 * Writes a single bit to the 1-Wire bus.
 *
 * :param val: True if we should write a 1, false for a 0.
 */
static void w1_write_bit(bool val)
{
	__disable_irq();
	/* Pull low for 6µs for 1, 60µs for 0 */
	gpio_set_output(w1_gpio, false);
	if (val)
		dwt_delay(6);
	else
		dwt_delay(60);
	/* Release to make up to 70µs total */
	gpio_set_input(w1_gpio);
	if (val)
		dwt_delay(64);
	else
		dwt_delay(10);
	__enable_irq();
}

void w1_write(uint8_t val)
{
	uint8_t i;

	for (i = 0; i < 8; i++) {
		__disable_irq();
		/* Pull low for 6µs for 1, 60µs for 0 */
		gpio_set_output(w1_gpio, false);
		if (val & 1)
			dwt_delay(6);
		else
			dwt_delay(60);
		/* Release to make up to 70µs total */
		gpio_set_input(w1_gpio);
		if (val & 1)
			dwt_delay(64);
		else
			dwt_delay(10);
		__enable_irq();

		val >>= 1;
	}
}

bool w1_read_bit(void)
{
	bool val;

	__disable_irq();
	/* Pull low for 6µs */
	gpio_set_output(w1_gpio, false);
	dwt_delay(6);
	/* Release for 9µs */
	gpio_set_input(w1_gpio);
	dwt_delay(9);

	/* Read the line state */
	val = gpio_get(w1_gpio);
	__enable_irq();

	chopstx_usec_wait(55);

	return val;
}

uint8_t w1_read_byte(void)
{
	uint8_t i, val;

	val = 0;
	for (i = 0; i < 8; i++) {
		if (w1_read_bit())
			val |= (1 << i);
	}

	return val;
}

void w1_read(uint8_t *buf, uint8_t len)
{
	uint8_t i;

	for (i = 0; i < len; i++) {
		buf[i] = w1_read_byte();
	}
}

bool w1_reset(bool nowait)
{
	bool present;

	/* Pull low for 480µs */
	gpio_set_output(w1_gpio, false);
	chopstx_usec_wait(480);
	/* Release for 70µs */
	gpio_set_input(w1_gpio);
	chopstx_usec_wait(70);

	/* If there's a device present it'll have pulled the line low */
	present = !gpio_get(w1_gpio);

	/* Wait for reset to complete */
	if (!nowait)
		chopstx_usec_wait(410);

	return present;
}

/**
 * 1-Wire binary search algorithm. See Maxim Application Note 187:
 *
 * https://www.maximintegrated.com/en/design/technical-documents/app-notes/1/187.html
 *
 * Called by w1_find_first and w1_find_next.
 *
 * :param state: Pointer to a state structure for the search. Will be
 *               be updated for subsequent calls to this function.
 * :param devid: 8 byte buffer containing the last seen device, and used for
 *               storing the ROM ID of the found device. Should be reused
 *               for repetative calls to this function.
 * :return: True if a device was found, false otherwise.
 */
static bool w1_search(struct w1_search_state *state, uint8_t devid[8])
{
	int i, last_zero;
	bool cmp_id_bit, id_bit, search_direction;

	if (state->last_device_flag || !w1_reset(true)) {
		state->last_device_flag = false;
		state->last_discrepancy = 64;

		return false;
	}

	last_zero = 64;

	w1_reset(false);
	w1_write(state->search_type);

	for (i = 0; i < 64; i++) {
		if (i == state->last_discrepancy)
			search_direction = true;
		else if (i > state->last_discrepancy)
			search_direction = false;
		else
			search_direction =
				(devid[i >> 3] & 1 << (i & 7));

		id_bit = w1_read_bit();
		cmp_id_bit = w1_read_bit();

		/* No devices, clear state and return. */
		if (id_bit && cmp_id_bit) {
			state->last_device_flag = false;
			state->last_discrepancy = 64;

			return false;
		}

		if (!id_bit && !cmp_id_bit) {
			/* Both bits valid, go the specified direction */
			w1_write_bit(search_direction);
		} else {
			/* Only one bit valid, take that direction */
			search_direction = id_bit;
			w1_write_bit(search_direction);
		}

		if (!id_bit && !cmp_id_bit && !search_direction)
			last_zero = i;

		if (search_direction)
			devid[i >> 3] |= 1 << (i & 7);
		else
			devid[i >> 3] &= ~(1 << (i & 7));
	}

	state->last_discrepancy = last_zero;
	if (last_zero == 64) {
		state->last_device_flag = true;
	}

	return (i == 64);
}

/**
 * This function finds the first 1-Wire device on the bus. The state and devid
 * will be initialised and left ready for subsequent calls to w1_find_next.
 *
 * :param state: Pointer to a state structure for the search. Will be
 *               initialised by this function.
 * :param devid: 8 byte buffer to store the ROM ID of the first found device.
 * :return: True if a device was found, false otherwise.
 */
bool w1_find_first(uint8_t cmd, struct w1_search_state *state,
		uint8_t devid[8])
{
	state->last_device_flag = false;
	state->last_discrepancy = 64;
	state->search_type = cmd;
	memset(devid, 0, 8);

	return w1_search(state, devid);
}

/**
 * This function finds the next 1-Wire device on the bus. The state and devid
 * must have been initialised by a call to w1_find_first() and will be updated
 * ready for the next call to w1_find_next.
 *
 * :param state: Pointer to a state structure for the search.
 * :param devid: 8 byte buffer that should contain the last found device on
 *               calling and returns the ROM ID of the next found device.
 * :return: True if a device was found, false otherwise.
 */
bool w1_find_next(struct w1_search_state *state, uint8_t devid[8])
{
	return w1_search(state, devid);
}

void w1_init(uint8_t gpio)
{
	w1_gpio = gpio;
	/* Set 1w pin to low */
	gpio_set(w1_gpio, false);
	/* Set 1w pin to input to let it float high */
	gpio_set_input(w1_gpio);
}
