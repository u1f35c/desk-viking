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

void gpio_set_direction(uint8_t gpio, bool input)
{
	struct GPIO *bank = gpio_get_base(gpio);
	uint32_t reg;
	int shift;

	if (!bank)
		return;

	reg = (gpio & 8) ? bank->CRH : bank->CRL;

	/* 4 bits per GPIO */
	shift = (gpio & 7) << 2;

	if (input) {
		reg &= ~(7 << shift);
		reg |= (8 << shift);
	} else {
		reg &= ~(12 << shift);
		reg |= (3 << shift);
	}

	if (gpio & 8) {
		bank->CRH = reg;
	} else {
		bank->CRL = reg;
	}
}

void gpio_set_opendrain(uint8_t gpio, bool open)
{
	struct GPIO *bank = gpio_get_base(gpio);
	uint32_t reg;
	int shift;

	if (!bank)
		return;

	reg = (gpio & 8) ? bank->CRH : bank->CRL;

	/* 4 bits per GPIO */
	shift = (gpio & 7) << 2;

	if (open) {
		reg |= (4 << shift);
	} else {
		reg &= ~(11 << shift);
	}

	if (gpio & 8) {
		bank->CRH = reg;
	} else {
		bank->CRL = reg;
	}
}

bool gpio_get_direction(uint8_t gpio)
{
	struct GPIO *bank = gpio_get_base(gpio);
	uint32_t reg;

	if (!bank)
		return false;

	reg = (gpio & 8) ? bank->CRH : bank->CRL;
	/* 4 bits per GPIO */
	reg >>= (gpio & 7) << 2;

	return !!(reg & 8);
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
