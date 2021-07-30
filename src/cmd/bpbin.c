// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Bus Pirate raw mode support
 *
 * http://dangerousprototypes.com/docs/Bitbang
 *
 * Copyright 2021 Jonathan McDowell <noodles@earth.li>
 */

#include "bpbin.h"
#include "buspirate.h"
#include "cdc.h"
#include "debug.h"
#include "gpio.h"

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
				resp = 0x80 | bp_read_state();
				bp_set_state(buf[i]);
				cdc_send(tty, (char *) &resp, 1);
			} else if ((buf[i] & 0xE0) == 0x40) {
				/* Configure pins as input/output */
				bp_set_direction(buf[i]);
				resp = 0x40 | bp_read_state();
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
					debug_print("Entering Bus Pirate binary I2C mode.\r\n");
					bpbin_i2c(tty, buf);
					bpbin_send_bbio1(tty);
					break;
				case 3:
					/* UART */
				case 4:
					/* 1-Wire */
					debug_print("Entering Bus Pirate binary 1-Wire mode.\r\n");
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
