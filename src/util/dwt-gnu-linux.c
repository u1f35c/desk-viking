// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * DWT delay routines for Linux emulation mode
 *
 * Copyright 2021 Jonathan McDowell <noodles@earth.li>
 */
#include <stdint.h>
#include <unistd.h>

#include "dwt.h"

/* Busy wait for a certain number of µs using the DWT counter */
void dwt_delay(uint16_t us)
{
	usleep(us);
}

/* Initialise and reset the DWT counter */
void dwt_init(void)
{
}
