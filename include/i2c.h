// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Basic routines to bit-bang standard I2C via a pair of GPIO pins
 *
 * Copyright 2020 Jonathan McDowell <noodles@earth.li>
 */
#ifndef __I2C_H__
#define __I2C_H__

void i2c_start(void);
void i2c_stop(void);
bool i2c_read_bit(void);
void i2c_write_bit(bool bit);
uint8_t i2c_read(void);
bool i2c_write(uint8_t val);
void i2c_init(uint8_t scl, uint8_t sda);

#endif /* __I2C_H__ */
