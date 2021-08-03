// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Definitions common to Bus Pirate binary protocol handlers
 *
 * Copyright 2021 Jonathan McDowell <noodles@earth.li>
 */
#ifndef __BPBIN_H__
#define __BPBIN_H__

#include "cdc.h"

void bpbin_err(struct cdc *tty);
void bpbin_ok(struct cdc *tty);
void bpbin_i2c(struct cdc *tty, uint8_t *buf);
void bpbin_raw(struct cdc *tty, uint8_t *buf);
void bpbin_w1(struct cdc *tty, uint8_t *buf);

#endif /* __BPBIN_H__ */
