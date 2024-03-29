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
#ifdef GNU_LINUX_EMULATION
#include <stdio.h>
#endif

#include <usb_lld.h>

#include "cdc.h"
#include "debug.h"
#include "dwt.h"
#include "gpio.h"
#include "util.h"
#include "version.h"

#define PRIO_CDC 2

#define STACK_MAIN
#define STACK_PROCESS_1
#include "stack-def.h"
#define STACK_ADDR_CDC ((uintptr_t)process1_base)
#define STACK_SIZE_CDC (sizeof process1_base)

bool bpbin_main(struct cdc *tty);
bool cli_main(struct cdc *tty, const uint8_t *s, int len);
void ccproxy_main(struct cdc *tty, const uint8_t *s, int len);

#ifdef GNU_LINUX_EMULATION
int emulated_main(int argc, const char *argv[])
#else
int main(int argc, const char *argv[])
#endif
{
	unsigned int zerocnt;
	struct cdc *tty;
	uint8_t count;
	uint8_t i;

	(void)argc;
	(void)argv;

#ifdef GNU_LINUX_EMULATION
	printf("Desk Viking " VER_STRING " (emulation with USBIP), a Bus Pirate inspired debug tool.\n");
#endif

	chopstx_usec_wait(200*1000);
	dwt_init();

	/* Reset everything back to input */
	bv_gpio_init();

	/* Setup our USB CDC ACM devices */
	cdc_init(PRIO_CDC, STACK_ADDR_CDC, STACK_SIZE_CDC, NULL, NULL);
	cdc_wait_configured();

	/* Debug TTY initialisation */
	debug_init();

	/* Open our main command TTY */
	tty = cdc_open(0);

	count = 0;
	while (1) {
		uint8_t buf[CDC_BUFSIZE];

		debug_print("Waiting for connection.\r\n");
		cdc_connected(tty, true);

		chopstx_usec_wait(50*1000);

		/* Send ZLP at the beginning.  */
		cdc_send(tty, buf, 0);

		/* "Got connection: xx\r\n" == 20 bytes */
		memcpy(buf, "Got connection: xx\r\n", 20);
		buf[16] = util_hexchar(count >> 4);
		buf[17] = util_hexchar(count & 0x0f);
		buf[20] = 0;
		count++;

		debug_print((char *)buf);

		zerocnt = 0;
		while (1) {
			int size;
			uint32_t usec;

			usec = 3000000;	/* 3.0 seconds */
			size = cdc_recv(tty, buf, &usec);
			/* Disconnection */
			if (size < 0)
				break;

			if (buf[0] == 0xF0) {
				/* CCLib Proxy mode */
				debug_print("Entering CCLib proxy mode.\r\n");
				ccproxy_main(tty, buf, size);
			} else {
				/* Bus Pirate modes; 1 == cli, 2 == raw, 0 == ignore */
				int mode = 0;

				if (buf[0] == '\r') {
					/* Bus Pirate-like CLI if user hits enter */
					zerocnt = 0;
					mode = 1; /* Interactive */
				} else if (buf[0] == 0) {
					/* Bus Pirate raw mode after 20 NULs */
					for (i = 0; i < size && zerocnt < 20; i++) {
						if (buf[i] == 0) {
							zerocnt++;
						} else {
							zerocnt = 0;
						}
					}
					if (zerocnt == 20) {
						mode = 2; /* Raw */
						zerocnt = 0;
					}
				} else {
					zerocnt = 0;
				}

				while (mode != 0) {
					if (mode == 1) {
						debug_print("Entering interactive mode.\r\n");
						mode = cli_main(tty, buf, size) ? 2 : 0;
					} else if (mode == 2) {
						debug_print("Entering Bus Pirate binary mode.\r\n");
						mode = bpbin_main(tty) ? 1 : 0;
					}
				}
			}
		}
	}

	return 0;
}
