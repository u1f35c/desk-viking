// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Bus Pirate binary raw mode support
 *
 * http://dangerousprototypes.com/docs/Raw-wire_(binary)
 *
 * Copyright 2021 Jonathan McDowell <noodles@earth.li>
 */

#include <stdbool.h>
#include <stdint.h>

#include "bpbin.h"
#include "buspirate.h"
#include "cdc.h"
#include "debug.h"
#include "dwt.h"
#include "gpio.h"
#include "intr.h"

struct bp_raw_conf {
	bool bigendian;
	bool hiz;
	bool raw2wire;
	int delay;
};

static void bpbin_send_raw1(struct cdc *tty)
{
	cdc_send(tty, "RAW1", 4);
}

static void bpbin_raw_clock_tick(struct bp_raw_conf *conf)
{
	gpio_set(PIN_CLK, true);
	dwt_delay(conf->delay);
	gpio_set(PIN_CLK, false);
	dwt_delay(conf->delay);
}

static void bpbin_raw_start(struct bp_raw_conf *conf)
{
	__disable_irq();
	gpio_set(PIN_MOSI, true);
	gpio_set(PIN_CLK, true);
	dwt_delay(conf->delay);
	gpio_set(PIN_MOSI, false);
	dwt_delay(conf->delay);
	gpio_set(PIN_CLK, false);
	dwt_delay(conf->delay);
	__enable_irq();
}

static void bpbin_raw_stop(struct bp_raw_conf *conf)
{
	__disable_irq();
	gpio_set(PIN_MOSI, false);
	dwt_delay(conf->delay);
	gpio_set(PIN_CLK, true);
	// TODO: Clock stretching
	dwt_delay(conf->delay);
	gpio_set(PIN_MOSI, true);
	dwt_delay(conf->delay);
	__enable_irq();
}


static bool bpbin_raw_read_bit(struct bp_raw_conf *conf)
{
	bool val;

	gpio_set_input(conf->raw2wire ? PIN_MOSI : PIN_MISO);
	gpio_set(PIN_CLK, true);
	dwt_delay(conf->delay);
	val = gpio_get(conf->raw2wire ? PIN_MOSI : PIN_MISO);
	gpio_set(PIN_CLK, false);
	dwt_delay(conf->delay);

	return val;
}

static uint8_t bpbin_raw_read(struct bp_raw_conf *conf)
{
	int i;
	uint8_t val;
	int pin = conf->raw2wire ? PIN_MOSI : PIN_MISO;

	gpio_set_input(pin);

	val = 0;
	for (i = 0; i < 8; i++) {
		gpio_set(PIN_CLK, true);
		dwt_delay(conf->delay);

		if (conf->bigendian) {
			val <<= 1;
			if (gpio_get(pin)) {
				val |= 1;
			}
		} else {
			val >>= 1;
			if (gpio_get(pin)) {
				val |= 0x80;
			}
		}

		gpio_set(PIN_CLK, false);
		dwt_delay(conf->delay);
	}

	return val;
}

static bool bpbin_raw_write(struct bp_raw_conf *conf, uint8_t val)
{
	int i;
	uint8_t read;

	gpio_set_output(PIN_MOSI, conf->hiz);

	read = 0;
	for (i = 0; i < 8; i++) {
		gpio_set(PIN_MOSI, val & (conf->bigendian ? 0x80 : 1));

		gpio_set(PIN_CLK, true);
		dwt_delay(conf->delay);

		if (conf->bigendian) {
			val <<= 1;
			read <<= 1;
			if (!conf->raw2wire && gpio_get(PIN_MISO)) {
				read |= 1;
			}
		} else {
			val >>= 1;
			read >>= 1;
			if (!conf->raw2wire && gpio_get(PIN_MISO)) {
				read |= 0x80;
			}
		}

		gpio_set(PIN_CLK, false);
		dwt_delay(conf->delay);
	}

	return read;
}

static void bpbin_raw_set_speed(struct bp_raw_conf *conf, int speed)
{
	switch (speed) {
	case 0:
		conf->delay = 100; /* ~ 5kHz */
		break;
	case 1:
		conf->delay = 10; /* ~ 50kHz */
		break;
	case 2:
		conf->delay = 5; /* ~ 100kHz */
		break;
	case 3:
		conf->delay = 1; /* ~ 400kHz */
		break;
	default:
		conf->delay = 5; /* ~ 100kHz */
	}
}

void bpbin_raw(struct cdc *tty, char *buf)
{
	int i, len;
	uint8_t resp;
	struct bp_raw_conf conf;

	conf.raw2wire = true;
	conf.bigendian = true;
	conf.hiz = true;
	conf.delay = 5; /* ~ 100Hz */

	/* CLK, output mode, open drain */
	gpio_set_output(PIN_CLK, true);
	/* MOSI, output mode, open drain */
	gpio_set_output(PIN_MOSI, true);
	/* MISO, input mode */
	gpio_set_input(PIN_MISO);

	bpbin_send_raw1(tty);

	while (1) {
		len = cdc_recv(tty, buf, NULL);
		if (len < 0)
			return;

		for (i = 0; i < len; i++) {
			if (buf[i] == 0) {
				/* Exit back to raw bitbang mode */
				return;
			} else if (buf[i] == 1) {
				bpbin_send_raw1(tty);
			} else if (buf[i] == 2) {
				/* I2C-style start */
				bpbin_raw_start(&conf);
				bpbin_ok(tty);
			} else if (buf[i] == 3) {
				/* I2C-style stop */
				bpbin_raw_stop(&conf);
				bpbin_ok(tty);
			} else if ((buf[i] & 0xFE) == 4) {
				/* Set CS */
				gpio_set(PIN_CS, buf[i] & 0x1);
				bpbin_ok(tty);
			} else if (buf[i] == 6) {
				/* Read byte */
				resp = bpbin_raw_read(&conf);
				cdc_send(tty, (char *) &resp, 1);
			} else if (buf[i] == 7) {
				/* Read bit */
				resp = bpbin_raw_read_bit(&conf) ? 1 : 0;
				cdc_send(tty, (char *) &resp, 1);
			} else if (buf[i] == 8) {
				/* Peek at input pin */
				if (conf.raw2wire)
					gpio_set_input(PIN_MOSI);
				resp = gpio_get(conf.raw2wire ? PIN_MOSI :
							PIN_MISO);
				cdc_send(tty, (char *) &resp, 1);
				bpbin_ok(tty);
			} else if (buf[i] == 9) {
				/* Clock tick */
				bpbin_raw_clock_tick(&conf);
				bpbin_ok(tty);
			} else if ((buf[i] & 0xFE) == 10) {
				/* Set clock */
				gpio_set(PIN_CLK, buf[i] & 0x1);
				bpbin_ok(tty);
			} else if ((buf[i] & 0xFE) == 12) {
				/* Set data */
				gpio_set(PIN_MOSI, buf[i] & 0x1);
				bpbin_ok(tty);
			} else if ((buf[i] & 0xF0) == 0x10) {
				/* Send 1-16 bytes */
				int left = (buf[i] & 0xF) + 1;

				bpbin_ok(tty);

				while (left) {
					i++;
					if (i < len) {
						resp = bpbin_raw_write(&conf, buf[i]);
						if (conf.raw2wire)
							resp = 1;
						cdc_send(tty, (char *) &resp, 1);
						left--;
					} else {
						len = cdc_recv(tty, buf, NULL);
						if (len < 0)
							return;
						i = 0;
					}
				}
			} else if ((buf[i] & 0xF0) == 0x20) {
				/* Send 1-16 clock ticks */
				for (int j = (buf[i] & 0xF) + 1; j > 0; j--) {
					bpbin_raw_clock_tick(&conf);
				}
				bpbin_ok(tty);
			} else if ((buf[i] & 0xF0) == 0x40) {
				/* Configure peripheral pins */
				bp_cfg_extra_pins(buf[i] & 0xF);
				bpbin_ok(tty);
			} else if ((buf[i] & 0xFC) == 0x60) {
				/* Set speed */
				bpbin_raw_set_speed(&conf, buf[i] & 0x3);
				bpbin_ok(tty);
			} else if ((buf[i] & 0xF0) == 0x80) {
				/* Configuration */
				conf.hiz = !(buf[i] & 8);
				gpio_set_output(PIN_MOSI, conf.hiz);
				gpio_set_output(PIN_CLK, conf.hiz);
				conf.raw2wire = !(buf[i] & 4);
				conf.bigendian = !(buf[i] & 2);
				bpbin_ok(tty);
			} else {
				bpbin_err(tty);
			}
		}
	}
}
