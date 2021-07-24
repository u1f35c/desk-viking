// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Bus Pirate raw mode support
 *
 * http://dangerousprototypes.com/docs/Bitbang
 *
 * Copyright 2021 Jonathan McDowell <noodles@earth.li>
 */

#include "bpbin.h"
#include "cdc.h"
#include "debug.h"
#include "gpio.h"

void bpbin_cfg_pins(uint8_t cfg) {
	/* Power */

	/* Pull ups */

	/* AUX */
	gpio_set(PIN_AUX, (cfg & 2));

	/* CS */
	gpio_set(PIN_CS, (cfg & 1));
}

void bpbin_err(struct cdc *tty)
{
	char resp = 0;
	cdc_send(tty, &resp, 1);
}

void bpbin_ok(struct cdc *tty)
{
	char resp = 1;
	cdc_send(tty, &resp, 1);
}

static void bpbin_send_bbio1(struct cdc *tty)
{
	cdc_send(tty, "BBIO1", 5);
}

static uint8_t bpbin_read_state(void)
{
	uint8_t resp;

	resp = gpio_get(PIN_AUX) ? 0x10 : 0;
	resp |= gpio_get(PIN_MOSI) ? 0x8 : 0;
	resp |= gpio_get(PIN_CLK) ? 0x4 : 0;
	resp |= gpio_get(PIN_MISO) ? 0x2 : 0;
	resp |= gpio_get(PIN_CS) ? 0x1 : 0;

	return resp;
}

static uint8_t bpbin_selftest(struct cdc *tty, char *buf, bool quick)
{
	int i, len;

	/* Fake all tests ok */
	bpbin_ok(tty);

	while ((len = cdc_recv(tty, buf, NULL)) >= 0) {
		for (i = 0; i < len; i++) {
			if (buf[i] == 0xFF)
				return 1;

			/* Fake all tests ok */
			bpbin_ok(tty);
		}
	}

	return 0;
}

bool bpbin_main(struct cdc *tty)
{
	char buf[CDC_BUFSIZE];
	int i, len;
	uint8_t resp;

	bpbin_send_bbio1(tty);

	while (1) {
		len = cdc_recv(tty, buf, NULL);
		if (len < 0)
			break;

		for (i = 0; i < len; i++) {
			if (buf[i] & 0x80) {
				/* Set/get pin status */

				resp = 0x80 | bpbin_read_state();

				/*
				 * Not yet implemented.
				 * gpio_set(PIN_POWER, buf[i] & 0x40);
				 * gpio_set(PIN_PULLUP, buf[i] & 0x20);
				 */
				gpio_set(PIN_AUX, buf[i] & 0x10);
				gpio_set(PIN_MOSI, buf[i] & 0x8);
				gpio_set(PIN_CLK, buf[i] & 0x4);
				gpio_set(PIN_MISO, buf[i] & 0x2);
				gpio_set(PIN_CS, buf[i] & 0x1);

				cdc_send(tty, (char *) &resp, 1);
			} else if ((buf[i] & 0xE0) == 0x40) {
				/* Configure pins as input/output */
				gpio_set_direction(PIN_AUX, buf[i] & 0x10);
				gpio_set_direction(PIN_MOSI, buf[i] & 0x8);
				gpio_set_direction(PIN_CLK, buf[i] & 0x4);
				gpio_set_direction(PIN_MISO, buf[i] & 0x2);
				gpio_set_direction(PIN_CS, buf[i] & 0x1);

				resp = 0x40 | bpbin_read_state();
				cdc_send(tty, (char *) &resp, 1);
			} else {
				switch(buf[i]) {
				case 0:
					bpbin_send_bbio1(tty);
					break;
				case 1:
					/* SPI */
				case 2:
					/* I2C */
				case 3:
					/* UART */
				case 4:
					/* 1-Wire */
					bpbin_w1(tty, buf);
					bpbin_send_bbio1(tty);
					break;
				case 5:
					/* Raw */
				case 6:
					/* OpenOCD JTAG */
				case 7:
				case 8:
				case 9:
				case 10:
				case 11:
				case 12:
				case 13:
				case 14:
					/* Unused */
					break;
				case 0xF:
					bpbin_ok(tty);
					/* Flag we want to drop to the CLI */
					return true;
				case 0x10:
				case 0x11:
					/* Self test mode */
					len = 0;
					resp = bpbin_selftest(tty, buf, buf[i] & 1);
					cdc_send(tty, (char *) &resp, 1);
					break;
				default:
					debug_print("Unknown raw mode command.\r\n");
					break;
				}
			}
		}
	}

	/* Disconnected or error, return to main command loop */
	return false;
}
