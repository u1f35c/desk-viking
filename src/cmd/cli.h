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
	MODE_MAX
};

struct cli_state {
	struct cdc *tty;
	enum cli_mode mode;
};

/* In cli_w1.c */
void cli_w1_setup(struct cli_state *state);
void cli_w1_start(struct cli_state *state);
void cli_w1_read(struct cli_state *state);
void cli_w1_write(struct cli_state *state, uint8_t val);

#endif /* __CLI_H__ */
