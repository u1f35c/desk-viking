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

void w1_write(uint8_t val)
{
	uint8_t i;

	for (i = 0; i < 8; i++) {
		__disable_irq();
		/* Pull low for 6µs for 1, 60µs for 0 */
		gpio_set_direction(w1_gpio, false);
		if (val & 1)
			dwt_delay(6);
		else
			dwt_delay(60);
		/* Release to make up to 70µs total */
		gpio_set_direction(w1_gpio, true);
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
	gpio_set_direction(w1_gpio, false);
	dwt_delay(6);
	/* Release for 9µs */
	gpio_set_direction(w1_gpio, true);
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
	gpio_set_direction(w1_gpio, false);
	chopstx_usec_wait(480);
	/* Release for 70µs */
	gpio_set_direction(w1_gpio, true);
	chopstx_usec_wait(70);

	/* If there's a device present it'll have pulled the line low */
	present = !gpio_get(w1_gpio);

	/* Wait for reset to complete */
	if (!nowait)
		chopstx_usec_wait(410);

	return present;
}

void w1_init(uint8_t gpio)
{
	w1_gpio = gpio;
	/* Set 1w pin to low */
	gpio_set(w1_gpio, false);
	/* Set 1w pin to input to let it float high */
	gpio_set_direction(w1_gpio, true);
}
