// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * DWT delay routines
 *
 * Busy waiting µs delay using the DWT counter
 *
 * Copyright 2020 Jonathan McDowell <noodles@earth.li>
 */
#ifndef __DWT_H__
#define __DWT_H__

#include <stdint.h>

void dwt_delay(uint16_t us);
void dwt_init(void);

#endif /* __DWT_H__ */
