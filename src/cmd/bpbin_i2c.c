// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Bus Pirate binary I2C mode support
 *
 * http://dangerousprototypes.com/docs/I2C_(binary)
 *
 * Copyright 2021 Jonathan McDowell <noodles@earth.li>
 */

#include "bpbin.h"
#include "buspirate.h"
#include "cdc.h"
#include "debug.h"
#include "gpio.h"
#include "i2c.h"

static void bpbin_send_i2c1(struct cdc *tty)
{
	cdc_send(tty, (uint8_t *) "I2C1", 4);
}

void bpbin_i2c(struct cdc *tty, uint8_t *buf)
{
	int i, len;
	uint8_t resp;

	i2c_init(PIN_CLK, PIN_MOSI);
	bpbin_send_i2c1(tty);

	while (1) {
		len = cdc_recv(tty, buf, NULL);
		if (len < 0)
			return;

		for (i = 0; i < len; i++) {
			if (buf[i] == 0) {
				/* Exit back to raw bitbang mode */
				return;
			} else if (buf[i] == 1) {
				bpbin_send_i2c1(tty);
			} else if (buf[i] == 2) {
				/* I2C start */
				i2c_start();
				bpbin_ok(tty);
			} else if (buf[i] == 3) {
				/* I2C stop */
				i2c_stop();
				bpbin_ok(tty);
			} else if (buf[i] == 4) {
				/* Read byte */
				resp = i2c_read();
				cdc_send(tty, &resp, 1);
			} else if (buf[i] == 6) {
				/* ACK bit */
				i2c_write_bit(false);
				bpbin_ok(tty);
			} else if (buf[i] == 7) {
				/* NACK bit */
				i2c_write_bit(true);
				bpbin_ok(tty);
			} else if ((buf[i] & 0xF0) == 0x10) {
				/* Send 1-16 bytes */
				int left = (buf[i] & 0xF) + 1;

				bpbin_ok(tty);

				while (left) {
					i++;
					if (i < len) {
						resp = i2c_write(buf[i]) ? 1 : 0;
						cdc_send(tty, &resp, 1);
						left--;
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
			} else if ((buf[i] & 0xFC) == 0x60) {
				/* Set speed */
			} else {
				bpbin_err(tty);
			}
		}
	}
}
