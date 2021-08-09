// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Definitions common to CLI protocol handlers
 *
 * Copyright 2020 Jonathan McDowell <noodles@earth.li>
 */
#ifndef __CLI_H__
#define __CLI_H__

#include <stdint.h>

#include "cdc.h"

enum cli_mode {
	MODE_HIZ = 0,
	MODE_1WIRE,
	MODE_I2C,
	MODE_DIO,
	MODE_MAX
};

struct cli_state {
	struct cdc *tty;
	enum cli_mode mode;
	/* Private work space for the current mode */
	uint8_t priv[16];
};

/* In cli_dio.c */
void cli_dio_read(struct cli_state *state);
void cli_dio_write(struct cli_state *state, uint8_t val);

/* In cli_i2c.c */
void cli_i2c_setup(struct cli_state *state);
void cli_i2c_start(struct cli_state *state);
void cli_i2c_stop(struct cli_state *state);
void cli_i2c_read(struct cli_state *state);
void cli_i2c_write(struct cli_state *state, uint8_t val);
bool cli_i2c_run_macro(struct cli_state *state, unsigned char macro);

/* In cli_w1.c */
void cli_w1_setup(struct cli_state *state);
void cli_w1_start(struct cli_state *state);
void cli_w1_read(struct cli_state *state);
void cli_w1_write(struct cli_state *state, uint8_t val);
bool cli_w1_run_macro(struct cli_state *state, unsigned char macro);

#endif /* __CLI_H__ */
