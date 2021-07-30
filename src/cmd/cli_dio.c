// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * CLI handlers for digital IO
 *
 * Copyright 2021 Jonathan McDowell <noodles@earth.li>
 */

#include "buspirate.h"
#include "tty.h"

#include "cli.h"

void cli_dio_read(struct cli_state *state)
{
	tty_putc(state->tty, ' ');
	tty_printhex(state->tty, bp_read_state(), 2);
}

void cli_dio_write(struct cli_state *state, uint8_t val)
{
	tty_putc(state->tty, ' ');
	tty_printhex(state->tty, val, 2);
	if (val & 0x80) {
		bp_set_state(val & 0x7F);
	} else {
		bp_set_direction(val);
	}
}
