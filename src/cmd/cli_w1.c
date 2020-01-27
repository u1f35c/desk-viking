// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * CLI handlers for 1-Wire protocol
 *
 * Copyright 2020 Jonathan McDowell <noodles@earth.li>
 */
#include "gpio.h"
#include "tty.h"
#include "w1.h"

#include "cli.h"

void cli_w1_setup(struct cli_state *state)
{
	(void)state;

	w1_init(PIN_MOSI);
}

void cli_w1_start(struct cli_state *state)
{
	tty_printf(state->tty, "BUS RESET: ");
	tty_printf(state->tty, w1_reset(false) ? "present\r\n" :
			"no device detected\r\n");
	// BUS RESET Warning: *No device detected
	// BUS RESET Warning: *Short or no pull-up
}

void cli_w1_read(struct cli_state *state)
{
	tty_putc(state->tty, ' ');
	tty_printhex(state->tty, w1_read_byte(), 2);
}

void cli_w1_write(struct cli_state *state, uint8_t val)
{
	tty_putc(state->tty, ' ');
	tty_printhex(state->tty, val, 2);
	w1_write(val);
}
