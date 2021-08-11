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

#define W1_READ_ROM	0x33
#define W1_ALARM_SEARCH	0xEC
#define W1_ROM_SEARCH	0xF0

enum w1_present_state {
	W1_NOT_PRESENT = 0,
	W1_PRESENT,
	W1_NO_PULLUP,
};

/**
 * Stores the state of an active 1-wire search process
 */
struct w1_search_state {
	/** True if we've found the last device */
	bool last_device_flag;
	/** Last bit position we had both high + low options */
	int last_discrepancy;
	/** Type of the search (W1_ALARM_SEARCH / W1_ROM_SEARCH) */
	uint8_t search_type;
};

uint8_t w1_crc(uint8_t *buf, uint8_t len);
void w1_write(uint8_t val);
bool w1_read_bit(void);
uint8_t w1_read_byte(void);
void w1_read(uint8_t *buf, uint8_t len);
enum w1_present_state w1_reset(bool nowait);
bool w1_find_first(uint8_t cmd, struct w1_search_state *state,
		uint8_t devid[8]);
bool w1_find_next(struct w1_search_state *state, uint8_t devid[8]);
void w1_init(uint8_t gpio);

#endif /* __W1_H__ */
