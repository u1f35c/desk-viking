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

#include <chopstx.h>
#include <sys.h>

#include "board.h"
#include "cdc.h"
#include "debug.h"
#include "gpio.h"
#include "tty.h"

static void cli_aux_read(struct cdc *tty)
{
	/* Set AUX to input */
	gpio_set_direction(PIN_AUX, true);
	tty_printf(tty, "AUX INPUT/HI-Z, READ: ");
	tty_putc(tty, gpio_get(PIN_AUX) ? '1' : '0');
	tty_printf(tty, "\r\n");
}

static void cli_aux_set(struct cdc *tty, bool on)
{
	/* Set AUX to output */
	gpio_set_direction(PIN_AUX, false);
	gpio_set(PIN_AUX, on);
	tty_printf(tty, on ? "AUX HIGH\r\n" : "AUX LOW\r\n");
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

static void cli_delay(struct cdc *tty, bool ms)
{
	if (ms) {
		tty_printf(tty, "DELAY 1ms\r\n");
		chopstx_usec_wait(1000);
	} else {
		tty_printf(tty, "DELAY 1µs\r\n");
		chopstx_usec_wait(1);
	}
}

static void cli_help(struct cdc *tty)
{
	tty_printf(tty, "General\r\n");
	tty_printf(tty, "--------------------------------\r\n");
	tty_printf(tty, "?      Help\r\n");
	tty_printf(tty, "#      Reset CLI state\r\n");
	tty_printf(tty, "&/%    Delay 1µs/ms\r\n");
	tty_printf(tty, "a/A/@  Set AUX low/HI/read value\r\n");
	tty_printf(tty, "i      Version/status info\r\n");
	tty_printf(tty, "v      Show volts/states\r\n");
}

static void cli_reset(struct cdc *tty)
{
	/* No-op for now, will reset back to HiZ eventually */
	tty_printf(tty, "RESET\r\n\r\n");
	cli_banner(tty);
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
	while (len > 0) {
		switch (*(cmd++)) {
		case ' ':
			/* Ignore */
			break;
		case '?':
			cli_help(tty);
			break;
		case '#':
			cli_reset(tty);
			break;
		case '&':
			cli_delay(tty, false);
			break;
		case '%':
			cli_delay(tty, true);
			break;
		case '@':
			cli_aux_read(tty);
			break;
		case 'a':
			cli_aux_set(tty, false);
			break;
		case 'A':
			cli_aux_set(tty, true);
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
		len--;
	}
}

void cli_main(struct cdc *tty, const char *s, int len)
{
	char cmd[65];
	unsigned int cmd_len;

	(void)s;
	(void)len;

	cli_banner(tty);

	while (1) {
		tty_printf(tty, ">");
		cmd_len = tty_readline(tty, cmd, sizeof(cmd));
		/* Process cmd */
		if (cmd_len > 0) {
			cmd[cmd_len] = 0;
			cli_process_cmd(tty, cmd, cmd_len);
			cmd_len = 0;
		} else if (cmd_len < 0) {
			break;
		}
	}

	debug_print("Leaving interactive mode.\r\n");
}
