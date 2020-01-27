// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * IRQ helpers
 *
 * Defines for enabling/disabling interrupts
 *
 * Copyright 2020 Jonathan McDowell <noodles@earth.li>
 */
#ifndef __INTR_H__
#define __INTR_H__

#define __disable_irq() asm volatile ("cpsid i" : : : "memory")
#define __enable_irq() asm volatile ("cpsie i" : : : "memory")

#endif /* __INTR_H__ */
