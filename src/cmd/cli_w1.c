// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * CLI handlers for 1-Wire protocol
 *
 * Copyright 2020 Jonathan McDowell <noodles@earth.li>
 */
#include "gpio.h"
#include "tty.h"
#include "w1.h"

#include "cli.h"

void cli_w1_setup(struct cli_state *state)
{
	(void)state;

	w1_init(PIN_MOSI);
}

void cli_w1_start(struct cli_state *state)
{
	enum w1_present_state present;

	tty_printf(state->tty, "BUS RESET: ");

	present = w1_reset(false);

	if (present == W1_PRESENT) {
		tty_printf(state->tty, "present\r\n");
	} else if (present == W1_NOT_PRESENT) {
		tty_printf(state->tty, "no device detected\r\n");
	} else if (present == W1_NO_PULLUP) {
		tty_printf(state->tty, "short or no-pullup\r\n");
	}
}

void cli_w1_read(struct cli_state *state)
{
	tty_putc(state->tty, ' ');
	tty_printhex(state->tty, w1_read_byte(), 2);
}

void cli_w1_write(struct cli_state *state, uint8_t val)
{
	tty_putc(state->tty, ' ');
	tty_printhex(state->tty, val, 2);
	w1_write(val);
}

static void cli_w1_print_devid(struct cli_state *state, uint8_t devid[8])
{
	int i;

	for (i = 0; i < 8; i++) {
		if (i != 0)
			tty_putc(state->tty, ' ');
		tty_printhex(state->tty, devid[i], 2);
	}
}

bool cli_w1_run_macro(struct cli_state *state, unsigned char macro)
{
	struct w1_search_state search;
	uint8_t devid[8];
	bool found;
	int i;

	switch (macro) {
	case 0:
		tty_printf(state->tty, "  0. Macro menu\r\n");
		tty_printf(state->tty,
			" 51. READ ROM (0x33) *for single device bus\r\n");
		tty_printf(state->tty,
			"236. ALARM SEARCH (0xEC)\r\n");
		tty_printf(state->tty,
			"240. ROM SEARCH (0xF0)\r\n");
		break;
	case 51:
		cli_w1_start(state);
		tty_printf(state->tty, "READ ROM (0x33): ");
		w1_write(W1_READ_ROM);
		for (i = 0; i < 8; i++) {
			devid[i] = w1_read_byte();
		}
		cli_w1_print_devid(state, devid);
		tty_printf(state->tty, "\r\n");
		break;
	case W1_ALARM_SEARCH:
	case W1_ROM_SEARCH:
		tty_printf(state->tty, "SEARCH (");
		tty_printhex(state->tty, macro, 2);
		tty_printf(state->tty, ")\r\n");
		tty_printf(state->tty, "Macro     1-Wire address\r\n");

		found = w1_find_first(macro, &search, devid);
		while (found) {
			tty_printf(state->tty, "          ");
			cli_w1_print_devid(state, devid);
			tty_printf(state->tty, "\r\n");

			found = w1_find_next(&search, devid);
		}

		tty_printf(state->tty, "Devices IDs are available by MACRO, see (0)\r\n");
		break;
	default:
		tty_printf(state->tty, "Unknown macro, try ? or (0) for help\r\n");
	}

	return true;
}
