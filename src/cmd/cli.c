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

#include "cli.h"

static void cli_hiz_setup(struct cli_state *state);

const struct cli_mode_info {
	char *name;
	void (*setup)(struct cli_state *);
	void (*start)(struct cli_state *);
	void (*stop)(struct cli_state *);
	void (*read)(struct cli_state *);
	void (*write)(struct cli_state *, uint8_t val);
} cli_modes[] = {
	{
		.name = "HiZ",
		.setup = cli_hiz_setup,
	},
	{
		.name = "1-Wire",
		.setup = cli_w1_setup,
		.start = cli_w1_start,
		.read = cli_w1_read,
		.write = cli_w1_write,
	},
	{
		.name = "I2C",
		.setup = cli_i2c_setup,
		.start = cli_i2c_start,
		.stop = cli_i2c_stop,
		.read = cli_i2c_read,
		.write = cli_i2c_write,
	},
	{
		.name = "DIO",
		.setup = cli_hiz_setup,
		.read = cli_dio_read,
		.write = cli_dio_write,
	},
};

static void cli_hiz_setup(struct cli_state *state)
{
	(void)state;
	/* Reset everything back to input */
	gpio_init();
}

static bool cli_aux_read(struct cli_state *state)
{
	/* Set AUX to input */
	gpio_set_input(PIN_AUX);
	tty_printf(state->tty, "AUX INPUT/HI-Z, READ: ");
	tty_putc(state->tty, gpio_get(PIN_AUX) ? '1' : '0');
	tty_printf(state->tty, "\r\n");

	return true;
}

static bool cli_aux_set(struct cli_state *state, bool on)
{
	/* Set AUX to output */
	gpio_set_output(PIN_AUX, false);
	gpio_set(PIN_AUX, on);
	tty_printf(state->tty, on ? "AUX HIGH\r\n" : "AUX LOW\r\n");

	return true;
}

static bool cli_banner(struct cli_state *state)
{
	tty_printf(state->tty, "DeskViking v0.1\r\n");
	tty_printf(state->tty, "Board name: " BOARD_NAME ", SYS Version: ");
	tty_putc(state->tty, sys_version[2]);
	tty_putc(state->tty, sys_version[4]);
	tty_putc(state->tty, sys_version[6]);
	tty_printf(state->tty, "\r\n");

	return true;
}

static bool cli_delay(struct cli_state *state, bool ms, unsigned int repeat)
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

	return true;
}

static bool cli_help(struct cli_state *state)
{
	tty_printf(state->tty, " General                              "
				"Protocol interaction\r\n");
	tty_printf(state->tty, " -------------------------------------"
				"-------------------------------------\r\n");
	tty_printf(state->tty, " ?      Help                          "
				"[/{    Start\r\n");
	tty_printf(state->tty, " #      Reset CLI state               "
				"]/}    Stop\r\n");
	tty_printf(state->tty, " &/%    Delay 1µs/ms                  "
				"123\r\n");
	tty_printf(state->tty, " a/A/@  Set AUX low/HI/read value     "
				"0x123  Send value\r\n");
	tty_printf(state->tty, " i      Version/status info           "
				"r      Read\r\n");
	tty_printf(state->tty, " m      Change mode                   "
				":      Repeat e.g. r:8\r\n");
	tty_printf(state->tty, " v      Show volts/states             "
				"\r\n");

	return true;
}

static bool cli_mode(struct cli_state *state)
{
	int i;
	char opt;
	int len;

	for (i = 0; i < MODE_MAX; i++) {
		tty_printdec(state->tty, i + 1);
		tty_printf(state->tty, ". ");
		tty_printf(state->tty, cli_modes[i].name);
		tty_printf(state->tty, "\r\n");
	}

	while (true) {
		tty_printf(state->tty, "(1)>");
		opt = '1';
		len = tty_readline(state->tty, &opt, 2);
		if (len < 0 || opt == 'x' || opt == 'X' ) {
			break;
		} else if (!isdigit(opt) || opt < '1' || opt > ('0' + MODE_MAX)) {
			tty_printf(state->tty, "Invalid choice, try again.\r\n");
		} else {
			state->mode = opt - '1';
			if (cli_modes[state->mode].setup)
				cli_modes[state->mode].setup(state);
			break;
		}
	}

	return true;
}

static bool cli_reset(struct cli_state *state)
{
	tty_printf(state->tty, "RESET\r\n\r\n");
	state->mode = MODE_HIZ;
	/* TODO: Reset back to all inputs */
	cli_hiz_setup(state);
	return cli_banner(state);
}

static bool cli_states(struct cli_state *state)
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

	return true;
}

/*
 * Handlers specific to the protocol in operation
 */

static bool cli_proto_null(struct cli_state *state)
{
	tty_printf(state->tty, "Error: Command has no effect here.\r\n");

	return false;
}

static bool cli_proto_start(struct cli_state *state)
{
	if (cli_modes[state->mode].start) {
		cli_modes[state->mode].start(state);
		return true;
	} else {
		return cli_proto_null(state);
	}
}

static bool cli_proto_stop(struct cli_state *state)
{
	if (cli_modes[state->mode].stop) {
		cli_modes[state->mode].stop(state);
		return true;
	} else {
		return cli_proto_null(state);
	}
}

static bool cli_proto_read(struct cli_state *state, unsigned int repeat)
{
	if (cli_modes[state->mode].read) {
		tty_printf(state->tty, "READ: ");
		while (repeat--)
			cli_modes[state->mode].read(state);
		tty_printf(state->tty, "\r\n");
		return true;
	} else {
		return cli_proto_null(state);
	}
}

static bool cli_proto_write(struct cli_state *state, unsigned int repeat, uint8_t val)
{
	if (cli_modes[state->mode].write) {
		tty_printf(state->tty, "WRITE: ");
		while (repeat--)
			cli_modes[state->mode].write(state, val);
		tty_printf(state->tty, "\r\n");
		return true;
	} else {
		return cli_proto_null(state);
	}
}

/*
 * Command line processing functions
 */

static unsigned int cli_parse_repeat(const char **cmd, unsigned int *len)
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
	char *end;
	uint8_t val;
	bool ok;

	pos = 0;
	while (len > 0) {
		ok = true;
		len--;
		pos++;
		switch (*(cmd++)) {
		case ' ':
		case ',':
			/* Ignore */
			break;
		case '?':
			ok = cli_help(state);
			break;
		case '#':
			ok = cli_reset(state);
			break;
		case '&':
			repeat = cli_parse_repeat(&cmd, &len);
			ok = cli_delay(state, false, repeat);
			break;
		case '%':
			repeat = cli_parse_repeat(&cmd, &len);
			ok = cli_delay(state, true, repeat);
			break;
		case '[':
		case '{':
			ok = cli_proto_start(state);
			break;
		case ']':
		case '}':
			ok = cli_proto_stop(state);
			break;
		case '@':
			ok = cli_aux_read(state);
			break;
		case 'a':
			ok = cli_aux_set(state, false);
			break;
		case 'A':
			ok = cli_aux_set(state, true);
			break;
		case 'i':
			ok = cli_banner(state);
			break;
		case 'm':
			ok = cli_mode(state);
			break;
		case 'r':
			repeat = cli_parse_repeat(&cmd, &len);
			ok = cli_proto_read(state, repeat);
			break;
		case 'v':
			ok = cli_states(state);
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			val = strtoul(cmd - 1, &end, 0);
			repeat = cli_parse_repeat(&cmd, &len);
			ok = cli_proto_write(state, repeat, val);
			len -= end - cmd;
			cmd = end;
			break;
		default:
			ok = false;
		}

		if (!ok) {
			tty_printf(state->tty, "Syntax error at char ");
			tty_printdec(state->tty, pos);
			tty_printf(state->tty, "\r\n");
			len = 0;
		}
	}
}

bool cli_main(struct cdc *tty, const char *s, int len)
{
	struct cli_state state;
	char cmd[65];
	int cmd_len;

	(void)s;
	(void)len;

	state.tty = tty;
	state.mode = MODE_HIZ;
	cli_hiz_setup(&state);

	cli_banner(&state);

	while (1) {
		tty_printf(tty, cli_modes[state.mode].name);
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
	return (cmd_len == TTY_BPRAW);
}
