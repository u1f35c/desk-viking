// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Basic routines to bit-bang standard 1-Wire via a GPIO pin
 *
 * Copyright 2018,2020 Jonathan McDowell <noodles@earth.li>
 */
#ifndef __W1_H__
#define __W1_H__

#include <stdbool.h>
#include <stdint.h>

uint8_t w1_crc(uint8_t *buf, uint8_t len);
void w1_write(uint8_t val);
bool w1_read_bit(void);
uint8_t w1_read_byte(void);
void w1_read(uint8_t *buf, uint8_t len);
bool w1_reset(bool nowait);
void w1_init(uint8_t gpio);

#endif /* __W1_H__ */
