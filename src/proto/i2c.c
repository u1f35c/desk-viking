// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Basic routines to bit-bang standard I2C via a pair of GPIO pins
 *
 * Copyright 2020 Jonathan McDowell <noodles@earth.li>
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <chopstx.h>

#include "dwt.h"
#include "gpio.h"
#include "i2c.h"
#include "intr.h"

static uint8_t i2c_scl, i2c_sda, i2c_speed;

void i2c_start(void)
{
	__disable_irq();
	gpio_set(i2c_sda, true);
	gpio_set(i2c_scl, true);
	dwt_delay(i2c_speed);
	gpio_set(i2c_sda, false);
	dwt_delay(i2c_speed);
	gpio_set(i2c_scl, false);
	dwt_delay(i2c_speed);
	__enable_irq();
}

void i2c_stop(void)
{
	__disable_irq();
	gpio_set(i2c_sda, false);
	dwt_delay(i2c_speed);
	gpio_set(i2c_scl, true);
	// TODO: Clock stretching
	dwt_delay(i2c_speed);
	gpio_set(i2c_sda, true);
	dwt_delay(i2c_speed);
	__enable_irq();
}

bool i2c_read_bit(void)
{
	bool bit;

	__disable_irq();
	/* SDA pulled high by pullup, allows slave to pull low */
	gpio_set(i2c_sda, true);

	dwt_delay(i2c_speed);

	gpio_set(i2c_scl, true);
	// TODO: Clock stretching
	dwt_delay(i2c_speed);
	bit = gpio_get(i2c_sda);

	gpio_set(i2c_scl, false);
	__enable_irq();

	return bit;
}

void i2c_write_bit(bool bit)
{
	__disable_irq();
	gpio_set(i2c_sda, bit);

	dwt_delay(i2c_speed);
	gpio_set(i2c_scl, true);
	dwt_delay(i2c_speed);
	// TODO: Clock stretching
	gpio_set(i2c_scl, false);
	__enable_irq();
}

uint8_t i2c_read(void)
{
	int i;
	uint8_t val;

	val = 0;
	for (i = 0; i < 8; i++) {
		val <<= 1;
		if (i2c_read_bit())
			val |= 1;
	}

	return val;
}

bool i2c_write(uint8_t val)
{
	int i;

	for (i = 0; i < 8; i++) {
		i2c_write_bit(val & 0x80);
		val <<= 1;
	}

	/* Read and return (n)ack */
	return i2c_read_bit();
}

/**
 * Performs a basic check to confirm that the i2c connection has working
 * pull up resistors by setting the open-drain outputs to high and confirming
 * that the result read from both is high.
 *
 * :return: True if the clock and data lines appear to have working pull up
 *          resistors.
 */
bool i2c_pullups_ok(void)
{
	/* Let the open-drain outputs be pulled high */
	gpio_set(i2c_sda, true);
	gpio_set(i2c_scl, true);
	dwt_delay(10);

	/* Pullups are only ok if both pins are now high */
	return gpio_get(i2c_sda) && gpio_get(i2c_scl);
}

void i2c_init(uint8_t scl, uint8_t sda)
{
	i2c_scl = scl;
	i2c_sda = sda;
	i2c_speed = 5; /* 100kHz */
	gpio_set_output(scl, true);
	gpio_set_output(sda, true);
}
