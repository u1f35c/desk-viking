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
	cdc_send(tty, (uint8_t *) str, strlen(str));
}

void tty_putc(struct cdc *tty, const char c)
{
	cdc_send(tty, (uint8_t *) &c, 1);
}

void tty_printbin(struct cdc *tty, int val)
{
	char buf[11];
	int i;

	buf[0] = '0';
	buf[1] = 'b';
	buf[10] = 0;

	for (i = 0; i < 8; i++) {
		buf[i + 2] = (val & 0x80) ? '1' : '0';
		val <<= 1;
	}

	tty_printf(tty, buf);
}

void tty_printdec(struct cdc *tty, int val)
{
	char buf[6];
	bool donedigit = false;
	int column = 10000;
	int digit;
	int pos = 0;

	while (column >= 10) {
		digit = val / column;
		if (donedigit || (digit > 0)) {
			buf[pos++] = digit + '0';
			val %= column;
			donedigit = true;
		}
		column /= 10;
	}
	buf[pos++] = val + '0';
	buf[pos] = 0;

	tty_printf(tty, buf);
}

void tty_printhex(struct cdc *tty, unsigned int val, int places)
{
	char buf[7];

	if (places == 0) {
		if (val > 0xFFF) {
			places = 4;
		} if (val > 0xFF) {
			places = 3;
		} if (val > 0xF) {
			places = 2;
		} else {
			places = 1;
		}
	}

	buf[0] = '0';
	buf[1] = 'x';
	places += 2;
	buf[places] = 0;

	while (places > 2) {
		--places;
		if ((val & 0xF) > 9) {
			buf[places] = (val & 0xF) - 10 + 'A';
		} else {
			buf[places] = (val & 0xF) + '0';
		}
		val >>= 4;
	}

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
		size = cdc_recv(tty, (uint8_t *) buf, &timeout);

		if (size < 0)
			return TTY_DISCONNECTED;

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

		/* 20 NULs in a row means drop to raw mode */
		if (len >= 20) {
			bool doraw = true;

			for (i = len-20; i < len; i++) {
				if (line[i] != 0) {
					doraw = false;
					break;
				}
			}
			if (doraw)
				return TTY_BPRAW;
		}
	}

	return len;
}
