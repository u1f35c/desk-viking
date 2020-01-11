// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * TTY over CDC helpers
 *
 * Helpers to provide interactive TTY functionality over the CDC ACM device.
 *
 * Copyright 2020 Jonathan McDowell <noodles@earth.li>
 */
#include <string.h>

#include "cdc.h"
#include "tty.h"

void tty_printf(struct cdc *tty, const char *str)
{
	cdc_send(tty, str, strlen(str));
}

void tty_putc(struct cdc *tty, const char c)
{
	cdc_send(tty, &c, 1);
}

void tty_printdec(struct cdc *tty, int val)
{
	char buf[6];
	bool donedigit = false;
	int column = 10000;
	int digit;
	int pos = 0;

	while (column > 10) {
		digit = val / column;
		if (donedigit || (digit > 0)) {
			buf[pos++] = digit + '0';
			val %= column;
		}
		column /= 10;
	}
	buf[pos++] = val + '0';
	buf[pos] = 0;

	tty_printf(tty, buf);
}

int tty_readline(struct cdc *tty, char *line, int linesize)
{
	int len = 0;

	while (1) {
		char buf[65];
		int i;
		int size;
		uint32_t timeout;

		timeout = 3000000; /* 3.0 seconds */
		size = cdc_recv(tty, buf, &timeout);

		if (size < 0)
			return -1;

		if (size > 0) {
			for (i = 0; i < size; i++) {
				switch (buf[i]) {
				case 0x0D: /* CR / Control-M */
					tty_printf(tty, "\r\n");
					/* Return line */
					return len;
				case 0x7F: /* DEL */
					if (len > 0) {
						tty_printf(tty, "\010 \010");
						len--;
					}
					break;
				default:
					if (len < (linesize - 1)) {
						tty_putc(tty, buf[i]);
						line[len++] = buf[i];
					} else {
						/* BEL */
						tty_putc(tty, 7);
					}
					break;
				}
			}
		}
	}

	return len;
}
