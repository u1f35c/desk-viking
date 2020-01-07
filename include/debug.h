// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Debug TTY support
 *
 * Uses a USB CDC ACM device to provide debugging messages
 *
 * Copyright 2020 Jonathan McDowell <noodles@earth.li>
 */
#ifndef __DEBUG_H__
#define __DEBUG_H__

void debug_print(const char *msg);
void debug_init(void);

#endif /* __DEBUG_H__ */
