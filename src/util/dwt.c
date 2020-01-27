// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * DWT delay routines
 *
 * Busy waiting µs delay using the DWT counter
 *
 * Copyright 2020 Jonathan McDowell <noodles@earth.li>
 */
#include <stdint.h>

#include "dwt.h"

/* DWT register layout */
struct DWT {
	volatile uint32_t CONTROL;
	volatile uint32_t CYCCNT;
};
static struct DWT *const DWT = (struct DWT *)0xE0001000;
#define DWTENA 0x8

static volatile uint32_t *DEMCR = (uint32_t *)0xE000EDFC;
#define TRCENA (1UL << 24)

/* Busy wait for a certain number of µs using the DWT counter */
void dwt_delay(uint16_t us)
{
	uint32_t start = DWT->CYCCNT;
	uint32_t count = us * 72;

	while ((DWT->CYCCNT - start) < count)
		;
}

/* Initialise and reset the DWT counter */
void dwt_init(void)
{
	*DEMCR |= TRCENA;
	DWT->CYCCNT = 0;
	DWT->CONTROL |= 1;
}
