// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * TTY over CDC helpers
 *
 * Helpers to provide interactive TTY functionality over the CDC ACM device.
 *
 * Copyright 2020 Jonathan McDowell <noodles@earth.li>
 */
#ifndef __TTY_H__
#define __TTY_H__

void tty_printf(struct cdc *tty, const char *str);
void tty_putc(struct cdc *tty, const char c);
void tty_printdec(struct cdc *tty, int val);
int tty_readline(struct cdc *tty, char *line, int linesize);

#endif /* __TTY_H__ */
