// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Underlying routines common to various Bus Pirate modes of operation.
 *
 * Copyright 2021 Jonathan McDowell <noodles@earth.li>
 */

#include <stdbool.h>
#include <stdint.h>

#include "buspirate.h"
#include "gpio.h"

void bp_cfg_extra_pins(uint8_t cfg)
{
	/* Power */

	/* Pull ups */

	/* AUX */
	gpio_set(PIN_AUX, (cfg & 2));

	/* CS */
	gpio_set(PIN_CS, (cfg & 1));
}

void bp_set_direction(uint8_t direction)
{
	/* Configure pins as input/output */
	if (direction & 0x10) {
		gpio_set_input(PIN_AUX);
	} else {
		gpio_set_output(PIN_AUX, false);
	}
	if (direction & 0x8) {
		gpio_set_input(PIN_MOSI);
	} else {
		gpio_set_output(PIN_MOSI, false);
	}
	if (direction & 0x4) {
		gpio_set_input(PIN_CLK);
	} else {
		gpio_set_output(PIN_CLK, false);
	}
	if (direction & 0x2) {
		gpio_set_input(PIN_MISO);
	} else {
		gpio_set_output(PIN_MISO, false);
	}
	if (direction & 0x1) {
		gpio_set_input(PIN_CS);
	} else {
		gpio_set_output(PIN_CS, false);
	}
}

uint8_t bp_read_state(void)
{
	uint8_t resp;

	resp = gpio_get(PIN_AUX) ? 0x10 : 0;
	resp |= gpio_get(PIN_MOSI) ? 0x8 : 0;
	resp |= gpio_get(PIN_CLK) ? 0x4 : 0;
	resp |= gpio_get(PIN_MISO) ? 0x2 : 0;
	resp |= gpio_get(PIN_CS) ? 0x1 : 0;

	return resp;
}

void bp_set_state(uint8_t state)
{
	gpio_set(PIN_AUX, state & 0x10);
	gpio_set(PIN_MOSI, state & 0x8);
	gpio_set(PIN_CLK, state & 0x4);
	gpio_set(PIN_MISO, state & 0x2);
	gpio_set(PIN_CS, state & 0x1);
}
