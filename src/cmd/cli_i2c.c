// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * CLI handlers for I2C protocol
 *
 * Copyright 2020 Jonathan McDowell <noodles@earth.li>
 */
#include "gpio.h"
#include "i2c.h"
#include "tty.h"

#include "cli.h"
#include "util.h"

struct i2c_state {
	bool ackpending;
};

void cli_i2c_setup(struct cli_state *state)
{
	struct i2c_state *ctx = (struct i2c_state *) state->priv;

	i2c_init(PIN_CLK, PIN_MOSI);
	ctx->ackpending = false;
}

void cli_i2c_start(struct cli_state *state)
{
	if (!i2c_pullups_ok())
		tty_printf(state->tty, "short or no-pullup\r\n");

	i2c_start();
	tty_printf(state->tty, "I2C START CONDITION\r\n");
}

void cli_i2c_stop(struct cli_state *state)
{
	struct i2c_state *ctx = (struct i2c_state *) state->priv;

	if (ctx->ackpending) {
		i2c_write_bit(true);
		ctx->ackpending = false;
		tty_printf(state->tty, "NACK\r\n");
	}
	i2c_stop();
	tty_printf(state->tty, "I2C STOP CONDITION\r\n");
}

void cli_i2c_read(struct cli_state *state)
{
	struct i2c_state *ctx = (struct i2c_state *) state->priv;

	if (ctx->ackpending) {
		i2c_write_bit(false);
		tty_printf(state->tty, " ACK");
	}

	tty_putc(state->tty, ' ');
	tty_printhex(state->tty, i2c_read(), 2);
	ctx->ackpending = true;
}

void cli_i2c_write(struct cli_state *state, uint8_t val)
{
	tty_putc(state->tty, ' ');
	tty_printhex(state->tty, val, 2);
	tty_printf(state->tty, i2c_write(val) ? " NACK" : " ACK");
}

static void cli_i2c_scan(struct cli_state *state)
{
	int i, j;
	char buf[5];

	if (!i2c_pullups_ok()) {
		tty_printf(state->tty, "short or no-pullup\r\n");
		return;
	}

	tty_printf(state->tty,
		"      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");

	for (i = 0; i < 128; i += 16) {
		buf[0] = util_hexchar(i >> 4);
		buf[1] = util_hexchar(i & 0xF);
		buf[2] = ':';
		buf[3] = ' ';
		buf[4] = 0;
		tty_printf(state->tty, buf);

		buf[0] = ' ';
		buf[1] = util_hexchar(i >> 4);
		buf[3] = 0;
		for (j = 0; j < 16; j++) {
			i2c_start();
			if (i2c_write((i + j) << 1)) {
				tty_printf(state->tty, " --");
			} else {
				buf[2] = util_hexchar(j);
				tty_printf(state->tty, buf);
			}
			i2c_write_bit(true);
			i2c_stop();
		}

		tty_printf(state->tty, "\r\n");
	}
}

bool cli_i2c_run_macro(struct cli_state *state, unsigned char macro)
{
	switch (macro) {
	case 0:
		tty_printf(state->tty, "  0. Macro menu\r\n");
		tty_printf(state->tty,
			"  1. 7-bit address search\r\n");
		break;
	case 1:
		tty_printf(state->tty, "Searching I2C address space:\r\n");
		cli_i2c_scan(state);
		break;
	default:
		tty_printf(state->tty, "Unknown macro, try ? or (0) for help\r\n");
	}

	return true;
}
