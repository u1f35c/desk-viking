// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * USB CDC driver
 *
 * Presents 2 ACM TTYs. Based on example-usb-serial from chopstx.
 *
 * Copyright 2013-2019 Flying Stone Technology
 * Copyright 2020 Jonathan McDowell <noodles@earth.li>
 */
#ifndef __CDC_H__
#define __CDC_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define BUFSIZE 64

struct cdc;

void cdc_init(uint16_t prio, uintptr_t stack_addr, size_t stack_size,
	  void (*sendbrk_callback) (uint8_t dev_no, uint16_t duration),
	  void (*config_callback) (uint8_t dev_no,
				   uint32_t bitrate, uint8_t format,
				   uint8_t paritytype, uint8_t databits));
void cdc_wait_configured(void);

struct cdc *cdc_open(uint8_t num);
bool cdc_connected(struct cdc *s, bool wait);
int cdc_send(struct cdc *s, const char *buf, int count);
int cdc_recv(struct cdc *s, char *buf, uint32_t *timeout);
int cdc_ss_notify(struct cdc *s, uint16_t state_bits);

#endif /* __CDC_H__ */
