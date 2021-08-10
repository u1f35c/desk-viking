// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * GPIO helpers
 *
 * Routines to access GPIOs
 *
 * Copyright 2020 Jonathan McDowell <noodles@earth.li>
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <mcu/stm32f103.h>

#include "gpio.h"

#define GPIO_CONF_MASK			0xF
#define GPIO_CONF_OUTPUT_PUSHPULL	0x1
#define GPIO_CONF_OUTPUT_OPENDRAIN	0x5
#define GPIO_CONF_INPUT_FLOATING	0x8

static struct GPIO *gpio_get_base(uint8_t gpio)
{
	switch (gpio >> 4) {
	case 0:
		return GPIOA;
	case 1:
		return GPIOB;
	case 2:
		return GPIOC;
	case 3:
		return GPIOD;
	case 4:
		return GPIOE;
	default:
		return NULL;
	}
}

/**
 * Sets the supplied pin to GPIO input mode, with no pull up/down enabled.
 *
 * :param gpio: GPIO pin to set to input mode
 */
void gpio_set_input(uint8_t gpio)
{
	struct GPIO *bank = gpio_get_base(gpio);
	uint32_t reg;
	int shift;

	if (!bank)
		return;

	reg = (gpio & 8) ? bank->CRH : bank->CRL;

	/* 4 bits per GPIO */
	shift = (gpio & 7) << 2;

	/* Clear the current configuration */
	reg &= ~(GPIO_CONF_MASK << shift);

	/* Set input mode */
	reg |= (GPIO_CONF_INPUT_FLOATING << shift);

	if (gpio & 8) {
		bank->CRH = reg;
	} else {
		bank->CRL = reg;
	}
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
	struct GPIO *bank = gpio_get_base(gpio);
	uint32_t reg;
	int shift;

	if (!bank)
		return;

	reg = (gpio & 8) ? bank->CRH : bank->CRL;

	/* 4 bits per GPIO */
	shift = (gpio & 7) << 2;

	/* Clear the current configuration */
	reg &= ~(GPIO_CONF_MASK << shift);

	if (open) {
		reg |= (GPIO_CONF_OUTPUT_OPENDRAIN << shift);
	} else {
		reg |= (GPIO_CONF_OUTPUT_PUSHPULL << shift);
	}

	if (gpio & 8) {
		bank->CRH = reg;
	} else {
		bank->CRL = reg;
	}
}

/**
 * Get the currently configured direction for the supplied GPIO pin.
 *
 * :param gpio: GPIO pin to query
 * :return: True if the supplied pin is in input mode, false otherwise */
bool gpio_get_direction(uint8_t gpio)
{
	struct GPIO *bank = gpio_get_base(gpio);
	uint32_t reg;

	if (!bank)
		return false;

	reg = (gpio & 8) ? bank->CRH : bank->CRL;
	/* 4 bits per GPIO */
	reg >>= (gpio & 7) << 2;

	reg &= GPIO_CONF_MASK;

	return (reg == GPIO_CONF_INPUT_FLOATING);
}

bool gpio_get(uint8_t gpio)
{
	struct GPIO *bank = gpio_get_base(gpio);

	if (!bank)
		return false;

	return !!(bank->IDR & (1 << (gpio & 15)));
}

void gpio_set(uint8_t gpio, bool on)
{
	struct GPIO *bank = gpio_get_base(gpio);

	if (!bank)
		return;

	if (on) {
		bank->BSRR |= 1 << (gpio & 15);
	} else {
		bank->BRR |= 1 << (gpio & 15);
	}
}

/**
 * Initialise all (aux, clock, CS, MISO + MOSI) GPIO pins to input mode.
 */
void bv_gpio_init(void)
{
	gpio_set_input(PIN_AUX);
	gpio_set_input(PIN_CLK);
	gpio_set_input(PIN_CS);
	gpio_set_input(PIN_MISO);
	gpio_set_input(PIN_MOSI);
#ifdef PIN_PULLUPS
	gpio_set(PIN_PULLUPS, false);
	gpio_set_output(PIN_PULLUPS, false);
#endif
#ifdef PIN_POWER
	gpio_set(PIN_POWER, false);
	gpio_set_output(PIN_POWER, false);
#endif
}
