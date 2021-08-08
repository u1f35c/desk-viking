// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * GPIO helpers
 *
 * Routines to access GPIOs
 *
 * Copyright 2020 Jonathan McDowell <noodles@earth.li>
 */
#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef GNU_LINUX_EMULATION

#define PIN_AUX		0
#define PIN_MOSI	1
#define PIN_CLK		2
#define PIN_MISO	3
#define PIN_CS		4
#define PIN_COUNT	5

#else

/* PB8 */
#define PIN_AUX		(16 + 8)
/* PB13 */
#define PIN_CLK		(16 + 13)
/* PB12 */
#define PIN_CS		(16 + 12)
/* PB14 */
#define PIN_MISO	(16 + 14)
/* PB15 */
#define PIN_MOSI	(16 + 15)

#endif

bool gpio_get_direction(uint8_t gpio);
void gpio_set_input(uint8_t gpio);
void gpio_set_output(uint8_t gpio, bool open);
bool gpio_get(uint8_t gpio);
void gpio_set(uint8_t gpio, bool on);
void bv_gpio_init(void);

#endif /* __GPIO_H__ */
