// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Debug TTY support
 *
 * Uses a USB CDC ACM device to provide debugging messages
 *
 * Copyright 2020 Jonathan McDowell <noodles@earth.li>
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <chopstx.h>

#include "cdc.h"
#include "debug.h"

static chopstx_mutex_t debug_mtx;
static struct cdc *debug_tty = NULL;

void debug_print(const char *msg)
{
	chopstx_mutex_lock(&debug_mtx);
	if (debug_tty && cdc_connected(debug_tty, false)) {
		cdc_send(debug_tty, (uint8_t *) msg, strlen(msg));
	}
	chopstx_mutex_unlock(&debug_mtx);
}

void debug_init(void)
{
	chopstx_mutex_init(&debug_mtx);
	debug_tty = cdc_open(1);
}
