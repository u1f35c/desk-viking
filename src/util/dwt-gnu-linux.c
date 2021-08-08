// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * DWT delay routines for Linux emulation mode
 *
 * Copyright 2021 Jonathan McDowell <noodles@earth.li>
 */
#include <stdint.h>
#include <unistd.h>

#include "dwt.h"

/* In gpio.c, but we don't want it generally visible */
void gpio_advance_clock(int us);

/**
 * Tell the GPIO module we waited for a certain number of µs
 */
void dwt_delay(uint16_t us)
{
	gpio_advance_clock(us);
}

/**
 * No-op initialisation of the DWT counter
 */
void dwt_init(void)
{
}
