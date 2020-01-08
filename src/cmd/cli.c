// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Bus Pirate inspired CLI access to Desk Viking functions
 *
 * Not intended to be a full implementation, but enough to be familiar.
 *
 * Copyright 2020 Jonathan McDowell <noodles@earth.li>
 */
#include <stdint.h>
#include <string.h>
#include <sys.h>

#include "board.h"
#include "cdc.h"
#include "debug.h"
#include "gpio.h"

void tty_printf(struct cdc *tty, const char *str)
{
	cdc_send(tty, str, strlen(str));
}

void tty_putc(struct cdc *tty, const char c)
{
	cdc_send(tty, &c, 1);
}

static void cli_banner(struct cdc *tty)
{
	tty_printf(tty, "DeskViking v0.1\r\n");
	tty_printf(tty, "Board name: " BOARD_NAME ", SYS Version: ");
	tty_putc(tty, sys_version[2]);
	tty_putc(tty, sys_version[4]);
	tty_putc(tty, sys_version[6]);
	tty_printf(tty, "\r\n");
}

static void cli_help(struct cdc *tty)
{
	tty_printf(tty, "General\r\n");
	tty_printf(tty, "-------------------\r\n");
	tty_printf(tty, "? Help\r\n");
	tty_printf(tty, "i Version/status info\r\n");
	tty_printf(tty, "v Show volts/states\r\n");
}

static void cli_states(struct cdc *tty)
{
	tty_printf(tty, "Pinstates:\r\n");
	tty_printf(tty, "GND\t3.3V\t5.0V\tADC\tVPU\tAUX\tCLK\tMOSI\tCS\tMISO\r\n");

	tty_printf(tty, "P\tP\tP\tI\tI\t");
	tty_printf(tty, gpio_get_direction(PIN_AUX) ? "I\t" : "O\t");
	tty_printf(tty, gpio_get_direction(PIN_CLK) ? "I\t" : "O\t");
	tty_printf(tty, gpio_get_direction(PIN_MOSI) ? "I\t" : "O\t");
	tty_printf(tty, gpio_get_direction(PIN_CS) ? "I\t" : "O\t");
	tty_printf(tty, gpio_get_direction(PIN_MISO) ? "I\t" : "O\t");
	tty_printf(tty, "\r\n");

	tty_printf(tty, "GND\t3.3V\t5.0V\t?\t?\t");
	tty_printf(tty, gpio_get(PIN_AUX) ? "H\t" : "L\t");
	tty_printf(tty, gpio_get(PIN_CLK) ? "H\t" : "L\t");
	tty_printf(tty, gpio_get(PIN_MOSI) ? "H\t" : "L\t");
	tty_printf(tty, gpio_get(PIN_CS) ? "H\t" : "L\t");
	tty_printf(tty, gpio_get(PIN_MISO) ? "H\t" : "L\t");
	tty_printf(tty, "\r\n");
}

static void cli_process_cmd(struct cdc *tty, const char *cmd, unsigned int len)
{
	(void)len;

	switch (cmd[0]) {
	case '?':
		cli_help(tty);
		break;
	case 'i':
		cli_banner(tty);
		break;
	case 'v':
		cli_states(tty);
		break;
	default:
		tty_printf(tty, "Unknown command.\r\n");
	}
}

void cli_main(struct cdc *tty, const char *s, int len)
{
	char cmd[65];
	unsigned int cmd_len;

	(void)s;
	(void)len;

	cli_banner(tty);

	tty_printf(tty, ">");
	cmd_len = 0;
	while (1) {
		char buf[65];
		int i;
		int size;
		uint32_t timeout;


		timeout = 3000000; /* 3.0 seconds */
		size = cdc_recv(tty, buf, &timeout);

		if (size < 0)
			return;

		if (size > 0) {
			for (i = 0; i < size; i++) {
				switch (buf[i]) {
				case 0x0D: /* CR / Control-M */
					tty_printf(tty, "\r\n");
					/* Process cmd */
					if (cmd_len > 0) {
						cmd[cmd_len] = 0;
						cli_process_cmd(tty, cmd, cmd_len);
						cmd_len = 0;
					}
					tty_printf(tty, ">");
					break;
				case 0x7F: /* DEL */
					if (cmd_len > 0) {
						tty_printf(tty, "\010 \010");
						cmd_len--;
					}
					break;
				default:
					if (cmd_len < (sizeof(cmd) - 1)) {
						tty_putc(tty, buf[i]);
						cmd[cmd_len++] = buf[i];
					} else {
						/* BEL */
						tty_putc(tty, 7);
					}
					break;
				}
			}
		}
	}

	debug_print("Leaving interactive mode.\r\n");
}
