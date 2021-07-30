// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Underlying routines common to various Bus Pirate modes of operation.
 *
 * Copyright 2021 Jonathan McDowell <noodles@earth.li>
 */

#ifndef __BUSPIRATE_H__
#define __BUSPIRATE_H__

#include <stdint.h>

void bp_cfg_extra_pins(uint8_t cfg);
void bp_set_direction(uint8_t direction);
uint8_t bp_read_state(void);
void bp_set_state(uint8_t state);

#endif /* __BUSPIRATE_H__ */
