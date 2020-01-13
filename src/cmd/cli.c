// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Bus Pirate inspired CLI access to Desk Viking functions
 *
 * Not intended to be a full implementation, but enough to be familiar.
 *
 * Copyright 2020 Jonathan McDowell <noodles@earth.li>
 */
#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include <chopstx.h>
#include <sys.h>

#include "board.h"
#include "cdc.h"
#include "debug.h"
#include "gpio.h"
#include "tty.h"

enum cli_mode {
	MODE_HIZ = 0,
	MODE_MAX
};

const char *cli_mode_str[] = {
	"HiZ",
};

struct cli_state {
	struct cdc *tty;
	enum cli_mode mode;
};

static void cli_aux_read(struct cli_state *state)
{
	/* Set AUX to input */
	gpio_set_direction(PIN_AUX, true);
	tty_printf(state->tty, "AUX INPUT/HI-Z, READ: ");
	tty_putc(state->tty, gpio_get(PIN_AUX) ? '1' : '0');
	tty_printf(state->tty, "\r\n");
}

static void cli_aux_set(struct cli_state *state, bool on)
{
	/* Set AUX to output */
	gpio_set_direction(PIN_AUX, false);
	gpio_set(PIN_AUX, on);
	tty_printf(state->tty, on ? "AUX HIGH\r\n" : "AUX LOW\r\n");
}

static void cli_banner(struct cli_state *state)
{
	tty_printf(state->tty, "DeskViking v0.1\r\n");
	tty_printf(state->tty, "Board name: " BOARD_NAME ", SYS Version: ");
	tty_putc(state->tty, sys_version[2]);
	tty_putc(state->tty, sys_version[4]);
	tty_putc(state->tty, sys_version[6]);
	tty_printf(state->tty, "\r\n");
}

static void cli_delay(struct cli_state *state, bool ms, unsigned int repeat)
{
	tty_printf(state->tty, "DELAY ");
	tty_printdec(state->tty, repeat);
	if (ms) {
		tty_printf(state->tty, "ms\r\n");
		chopstx_usec_wait(1000 * repeat);
	} else {
		tty_printf(state->tty, "µs\r\n");
		chopstx_usec_wait(repeat);
	}
}

static void cli_help(struct cli_state *state)
{
	tty_printf(state->tty, "General\r\n");
	tty_printf(state->tty, "--------------------------------\r\n");
	tty_printf(state->tty, "?      Help\r\n");
	tty_printf(state->tty, "#      Reset CLI state\r\n");
	tty_printf(state->tty, "&/%    Delay 1µs/ms\r\n");
	tty_printf(state->tty, "a/A/@  Set AUX low/HI/read value\r\n");
	tty_printf(state->tty, "i      Version/status info\r\n");
	tty_printf(state->tty, "v      Show volts/states\r\n");
}

static void cli_reset(struct cli_state *state)
{
	tty_printf(state->tty, "RESET\r\n\r\n");
	state->mode = MODE_HIZ;
	/* TODO: Reset back to all inputs */
	cli_banner(state);
}

static void cli_states(struct cli_state *state)
{
	tty_printf(state->tty, "Pinstates:\r\n");
	tty_printf(state->tty, "GND\t3.3V\t5.0V\tADC\tVPU\tAUX\tCLK\tMOSI\tCS\tMISO\r\n");

	tty_printf(state->tty, "P\tP\tP\tI\tI\t");
	tty_printf(state->tty, gpio_get_direction(PIN_AUX) ? "I\t" : "O\t");
	tty_printf(state->tty, gpio_get_direction(PIN_CLK) ? "I\t" : "O\t");
	tty_printf(state->tty, gpio_get_direction(PIN_MOSI) ? "I\t" : "O\t");
	tty_printf(state->tty, gpio_get_direction(PIN_CS) ? "I\t" : "O\t");
	tty_printf(state->tty, gpio_get_direction(PIN_MISO) ? "I\t" : "O\t");
	tty_printf(state->tty, "\r\n");

	tty_printf(state->tty, "GND\t3.3V\t5.0V\t?\t?\t");
	tty_printf(state->tty, gpio_get(PIN_AUX) ? "H\t" : "L\t");
	tty_printf(state->tty, gpio_get(PIN_CLK) ? "H\t" : "L\t");
	tty_printf(state->tty, gpio_get(PIN_MOSI) ? "H\t" : "L\t");
	tty_printf(state->tty, gpio_get(PIN_CS) ? "H\t" : "L\t");
	tty_printf(state->tty, gpio_get(PIN_MISO) ? "H\t" : "L\t");
	tty_printf(state->tty, "\r\n");
}

static int cli_parse_repeat(const char **cmd, unsigned int *len)
{
	unsigned int repeat, pos;

	if (*len < 2) {
		return 1;
	}
	if (**cmd != ':') {
		return 1;
	}

	repeat = 0;
	for (pos = 1; (pos < *len) && isdigit((*cmd)[pos]); pos++) {
		repeat *= 10;
		repeat += (*cmd)[pos] - '0';
	}

	if (repeat == 0) {
		return 1;
	}

	*len -= pos;
	*cmd += pos;
	return repeat;
}

static void cli_process_cmd(struct cli_state *state, const char *cmd, unsigned int len)
{
	unsigned int repeat, pos;

	pos = 0;
	while (len > 0) {
		len--;
		pos++;
		switch (*(cmd++)) {
		case ' ':
			/* Ignore */
			break;
		case '?':
			cli_help(state);
			break;
		case '#':
			cli_reset(state);
			break;
		case '&':
			repeat = cli_parse_repeat(&cmd, &len);
			cli_delay(state, false, repeat);
			break;
		case '%':
			repeat = cli_parse_repeat(&cmd, &len);
			cli_delay(state, true, repeat);
			break;
		case '@':
			cli_aux_read(state);
			break;
		case 'a':
			cli_aux_set(state, false);
			break;
		case 'A':
			cli_aux_set(state, true);
			break;
		case 'i':
			cli_banner(state);
			break;
		case 'v':
			cli_states(state);
			break;
		default:
			tty_printf(state->tty, "Syntax error at char ");
			tty_printdec(state->tty, pos);
			tty_printf(state->tty, "\r\n");
			len = 0;
		}
	}
}

void cli_main(struct cdc *tty, const char *s, int len)
{
	struct cli_state state;
	char cmd[65];
	int cmd_len;

	(void)s;
	(void)len;

	state.tty = tty;
	state.mode = MODE_HIZ;

	cli_banner(&state);

	while (1) {
		tty_printf(tty, cli_mode_str[state.mode]);
		tty_printf(tty, ">");
		cmd_len = tty_readline(tty, cmd, sizeof(cmd));
		/* Process cmd */
		if (cmd_len > 0) {
			cmd[cmd_len] = 0;
			cli_process_cmd(&state, cmd, cmd_len);
			cmd_len = 0;
		}
		if (cmd_len < 0)
			break;
	}

	debug_print("Leaving interactive mode.\r\n");
}
