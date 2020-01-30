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
