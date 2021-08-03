// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Bus Pirate binary 1-wire mode support
 *
 * http://dangerousprototypes.com/docs/1-Wire_(binary)
 *
 * Copyright 2021 Jonathan McDowell <noodles@earth.li>
 */

#include "bpbin.h"
#include "buspirate.h"
#include "cdc.h"
#include "debug.h"
#include "gpio.h"
#include "w1.h"

static void bpbin_send_1w10(struct cdc *tty)
{
	cdc_send(tty, (uint8_t *) "1W01", 4);
}

void bpbin_w1(struct cdc *tty, uint8_t *buf)
{
	int i, len;
	uint8_t resp;
	uint8_t search[8] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

	w1_init(PIN_MOSI);
	bpbin_send_1w10(tty);

	while (1) {
		len = cdc_recv(tty, buf, NULL);
		if (len < 0)
			return;

		for (i = 0; i < len; i++) {
			if (buf[i] == 0) {
				/* Exit back to raw bitbang mode */
				return;
			} else if (buf[i] == 1) {
				bpbin_send_1w10(tty);
			} else if (buf[i] == 2) {
				/* 1-Wire reset */
				w1_reset(false);
				bpbin_ok(tty);
			} else if (buf[i] == 4) {
				/* Read byte */
				w1_read(&resp, 1);
				cdc_send(tty, &resp, 1);
			} else if (buf[i] == 8) {
				/* ROM search (0xF0) */
				bpbin_ok(tty);
				cdc_send(tty, search, 8);
			} else if (buf[i] == 9) {
				/* ALARM search (0xEC) */
				bpbin_ok(tty);
				cdc_send(tty, search, 8);
			} else if ((buf[i] & 0xF0) == 0x10) {
				/* Send 1-16 bytes */
				int left = (buf[i] & 0xF) + 1;
				i++;
				while (left) {
					if (i < len) {
						w1_write(buf[i]);
						bpbin_ok(tty);
						left--;
						i++;
					} else {
						len = cdc_recv(tty, buf, NULL);
						if (len < 0)
							return;
						i = 0;
					}
				}
			} else if ((buf[i] & 0xF0) == 0x40) {
				/* Configure peripheral pins */
				bp_cfg_extra_pins(buf[i] & 0xF);
				bpbin_ok(tty);
			}
		}
	}
}
