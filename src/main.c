// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Desk Viking
 *
 * A Bus Pirate inspired debug device based on the STM32F103.
 *
 * Copyright 2020 Jonathan McDowell <noodles@earth.li>
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <chopstx.h>

#include <usb_lld.h>

#include "cdc.h"
#include "debug.h"

#define PRIO_CDC 2

#define STACK_MAIN
#define STACK_PROCESS_1
#include "stack-def.h"
#define STACK_ADDR_CDC ((uint32_t)process1_base)
#define STACK_SIZE_CDC (sizeof process1_base)

static char hexchar (uint8_t x)
{
	x &= 0x0f;
	if (x <= 0x09)
		return '0' + x;
	else if (x <= 0x0f)
		return 'a' + x - 10;
	else
		return '?';
}

void cli_main(struct cdc *tty, const char *s, int len);
void ccproxy_main(struct cdc *tty, const char *s, int len);

int main(int argc, const char *argv[])
{
	struct cdc *tty;
	uint8_t count;

	(void)argc;
	(void)argv;

	chopstx_usec_wait(200*1000);

	/* Setup our USB CDC ACM devices */
	cdc_init(PRIO_CDC, STACK_ADDR_CDC, STACK_SIZE_CDC, NULL, NULL);
	cdc_wait_configured();

	/* Debug TTY initialisation */
	debug_init();

	/* Open our main command TTY */
	tty = cdc_open(0);

	count = 0;
	while (1) {
		char s[70];

		debug_print("Waiting for connection.\r\n");
		cdc_connected(tty, true);

		chopstx_usec_wait (50*1000);

		/* Send ZLP at the beginning.  */
		cdc_send(tty, s, 0);

		memcpy(s, "xx: Hello, World with Chopstx!\r\n", 32);
		s[0] = hexchar (count >> 4);
		s[1] = hexchar (count & 0x0f);
		s[32] = 0;
		count++;

		debug_print(s);

		while (1) {
			int size;
			uint32_t usec;

			usec = 3000000;	/* 3.0 seconds */
			size = cdc_recv(tty, s + 4, &usec);
			/* Disconnection */
			if (size < 0)
				break;

			if (size) {
				s[0] = hexchar(size >> 4);
				s[1] = hexchar(size & 0x0f);
				s[2] = ':';
				s[3] = ' ';
				s[4 + size] = '\r';
				s[5 + size] = '\n';
				if (cdc_send(tty, s, size + 6) < 0)
					break;
			}
		}
	}

	return 0;
}
