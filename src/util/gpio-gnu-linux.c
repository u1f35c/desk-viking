// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * GPIO helpers for Linux emulation
 *
 * Routines to access GPIOs
 *
 * Copyright 2021 Jonathan McDowell <noodles@earth.li>
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "gpio.h"

/**
 * Sets the supplied pin to GPIO input mode, with no pull up/down enabled.
 *
 * :param gpio: GPIO pin to set to input mode
 */
void gpio_set_input(uint8_t gpio)
{
	(void)gpio;
}

/**
 * Sets the supplied pin to GPIO output mode. If open is true then the pin is
 * set to open-drain mode.
 *
 * :param gpio: GPIO pin to set to output mode
 * :param open: True if open-drain mode should be enabled, false for push-pull
 */
void gpio_set_output(uint8_t gpio, bool open)
{
	(void)gpio;
	(void)open;
}

/**
 * Get the currently configured direction for the supplied GPIO pin.
 *
 * :param gpio: GPIO pin to query
 * :return: True if the supplied pin is in input mode, false otherwise */
bool gpio_get_direction(uint8_t gpio)
{
	(void)gpio;

	return true;
}

bool gpio_get(uint8_t gpio)
{
	(void)gpio;

	return false;
}

void gpio_set(uint8_t gpio, bool on)
{
	(void)gpio;
	(void)on;
}

/**
 * Initialise all (aux, clock, CS, MISO + MOSI) GPIO pins to input mode.
 */
void bv_gpio_init(void)
{
}
