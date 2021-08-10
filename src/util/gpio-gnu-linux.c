// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * GPIO helpers for Linux emulation
 *
 * Routines to access GPIOs
 *
 * Copyright 2021 Jonathan McDowell <noodles@earth.li>
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "gpio.h"
#include "version.h"

#define USEC_IN_MSEC	1000L

/**
 * The modes that a pin can be in
 */
enum pin_mode {
	PIN_INPUT_FLOATING,
	PIN_OUTPUT_PUSHPULL,
	PIN_OUTPUT_OPENDRAIN,
};

/**
 * Static state for the GPIO pins we're emulating.
 */
static struct gpio_state {
	FILE *vcdfile;
	unsigned long ts;
	time_t last_output;

	struct pin_state {
		enum pin_mode mode;
		bool state;
	} pins[PIN_COUNT];

	char last_state[PIN_COUNT];
} state;

/**
 * Checks if the pull-up resistors are enabled and if the pin provided is one
 * of those attached to the pull-ups (MOSI/MISO/CLK/CS).
 *
 * :return True if the pull-ups are enabled and this is a pulled-up pin
 */
static bool gpio_has_pullup(uint8_t gpio)
{
	switch (gpio) {
	case PIN_CLK:
	case PIN_CS:
	case PIN_MISO:
	case PIN_MOSI:
		return state.pins[PIN_PULLUPS].state;
	default:
		return false;
	}
}

/**
 * Returns the state of the GPIO as a char, suitable for the VCD file.
 * Z indicates Hi-Z (i.e input) and for an output either 0 or 1 will be
 * returned. If the pin is an input or in open drain mode then the state
 * of any pull-up resistor is considered and Hi-Z returned otherwise.
 */
static char gpio_state_to_char(uint8_t gpio)
{
	if (state.pins[gpio].mode == PIN_INPUT_FLOATING)
		return gpio_has_pullup(gpio) ? '1' : 'Z';

	if (state.pins[gpio].mode == PIN_OUTPUT_OPENDRAIN) {
		if (!state.pins[gpio].state)
			return '0';
		if (gpio_has_pullup(gpio))
			return '1';

		return 'Z';
	}

	return state.pins[gpio].state ? '1' : '0';
}

/**
 * Checks if our GPIO pin state has changed since we last wrote it to the VCD
 * and if so writes the timestamp and the most recent changes.
 *
 * :return: True if we wrote an update to the VCD.
 */
static bool gpio_vcd_write_state(void)
{
	char *pin_tokens = "!\"#$%&'";
	bool changed;
	char s;
	int i;

	changed = false;
	for (i = 0; !changed && i < PIN_COUNT; i++) {
		if (state.last_state[i] != gpio_state_to_char(i)) {
			changed = true;
		}
	}

	if (!changed) {
		return false;
	}

	/* Store the time we last output our state */
	state.last_output = time(NULL);

	fprintf(state.vcdfile, "#%ld", state.ts);

	for (i = 0; i < PIN_COUNT; i++) {
		s = gpio_state_to_char(i);
		if (state.last_state[i] != s) {
			state.last_state[i] = s;
			fprintf(state.vcdfile, " %c%c", s, pin_tokens[i]);
		}
	}

	fprintf(state.vcdfile, "\n");

	return true;
}

/**
 * Checks if it's been more than a second since we last output our state and
 * if so calls gpio_vcd_write_state() to write the current state if it's
 * changed.
 */
static void gpio_periodic_check(void)
{
	time_t t;

	t = time(NULL);

	if ((state.last_output + 1) >= t) {
		return;
	}

	if (gpio_vcd_write_state()) {
		/* Bump the current clock up to the next millisecond */
		state.ts += 2 * USEC_IN_MSEC;
		state.ts -= (state.ts % USEC_IN_MSEC);
	}
}

/**
 * Sets the supplied pin to GPIO input mode, with no pull up/down enabled.
 *
 * :param gpio: GPIO pin to set to input mode
 */
void gpio_set_input(uint8_t gpio)
{
	if (gpio >= PIN_COUNT)
		return;

	gpio_periodic_check();

	state.pins[gpio].mode = PIN_INPUT_FLOATING;
}

/**
 * Sets the supplied pin to GPIO output mode. If open is true then the pin is
 * set to open-drain mode.
 *
 * :param gpio: GPIO pin to set to output mode
 * :param open: True if open-drain mode should be enabled, false for push-pull
 */
void gpio_set_output(uint8_t gpio, bool open)
{
	if (gpio >= PIN_COUNT)
		return;

	gpio_periodic_check();

	if (open) {
		state.pins[gpio].mode = PIN_OUTPUT_OPENDRAIN;
	} else {
		state.pins[gpio].mode = PIN_OUTPUT_PUSHPULL;
	}
}

/**
 * Get the currently configured direction for the supplied GPIO pin.
 *
 * :param gpio: GPIO pin to query
 * :return: True if the supplied pin is in input mode, false otherwise
 */
bool gpio_get_direction(uint8_t gpio)
{
	if (gpio >= PIN_COUNT)
		return false;

	return (state.pins[gpio].mode == PIN_INPUT_FLOATING);
}

/**
 * Returns the current state (high/low) of the supplied GPIO pin. Takes into
 * consideration the state of any pull-up resistors associated with the pin.
 *
 * :return: true if the pin is high, false otherwise
 */
bool gpio_get(uint8_t gpio)
{
	if (gpio >= PIN_COUNT)
		return false;

	if (state.pins[gpio].mode == PIN_INPUT_FLOATING) {
		return gpio_has_pullup(gpio);
	} else if (state.pins[gpio].mode == PIN_OUTPUT_OPENDRAIN) {
		return state.pins[gpio].state ? gpio_has_pullup(gpio) : false;
	} else {
		return state.pins[gpio].state;
	}
}

void gpio_set(uint8_t gpio, bool on)
{
	if (gpio >= PIN_COUNT)
		return;

	gpio_periodic_check();

	state.pins[gpio].state = on;
}

/**
 * Writes the current GPIO state (if it's changed since last time) and advances
 * the clock for the purposes of our VCD output.
 *
 * Called by the dwt_delay() function
 *
 * :param us: The time in µs to advance the clokc ticks by.
 */
void gpio_advance_clock(int us)
{
	gpio_vcd_write_state();
	state.ts += us;
}

/**
 * Cleans up our VCD file at program exit.
 */
static void gpio_exit(void)
{
	gpio_vcd_write_state();
	fclose(state.vcdfile);
}

/**
 * Initialise all (aux, clock, CS, MISO + MOSI) GPIO pins to input mode, and
 * setup the VCD file tracing.
 */
void bv_gpio_init(void)
{
	static bool done_setup = false;
	struct tm timespec;
	char buf[64];		/* Needs to be at least 26 bytes for ctime_r */
	time_t t;

	/* We only want to setup the VCD once per run */
	if (done_setup)
		return;

	memset(&state, 0, sizeof(state));

	state.pins[PIN_PULLUPS].state = false;
	state.pins[PIN_PULLUPS].mode = PIN_OUTPUT_PUSHPULL;
	state.pins[PIN_POWER].state = false;
	state.pins[PIN_POWER].mode = PIN_OUTPUT_PUSHPULL;

	t = time(NULL);
	localtime_r(&t, &timespec);
	snprintf(buf, sizeof(buf),
			"desk-viking-%04d%02d%02dT%02d%02d%02d.vcd",
			timespec.tm_year + 1900,
			timespec.tm_mon + 1,
			timespec.tm_mday,
			timespec.tm_hour,
			timespec.tm_min,
			timespec.tm_sec);

	state.vcdfile = fopen(buf, "w");

	if (state.vcdfile == NULL) {
		perror("Couldn't open VCD GPIO log file:");
		exit(1);
	}

	ctime_r(&t, buf);
	/* Remove the trailing \n */
	buf[strlen(buf) - 1] = 0;

	fprintf(state.vcdfile, "$date %s $end\n", buf);

	fprintf(state.vcdfile, "$version Desk Viking " VER_STRING " $end\n");
	fprintf(state.vcdfile, "$comment\n");
	fprintf(state.vcdfile,
		"  Debug tracefile from Desk Viking Linux emulation mode\n");
	fprintf(state.vcdfile, "$end\n");
	fprintf(state.vcdfile, "$timescale 1 us $end\n");
	fprintf(state.vcdfile, "$scope module desk-viking $end\n");

	fprintf(state.vcdfile, "$var wire 1 ! AUX $end\n");
	fprintf(state.vcdfile, "$var wire 1 \" MOSI $end\n");
	fprintf(state.vcdfile, "$var wire 1 # CLK $end\n");
	fprintf(state.vcdfile, "$var wire 1 $ MISO $end\n");
	fprintf(state.vcdfile, "$var wire 1 %% CS $end\n");
	fprintf(state.vcdfile, "$var wire 1 & PULLUPS $end\n");
	fprintf(state.vcdfile, "$var wire 1 ' POWER $end\n");

	fprintf(state.vcdfile, "$upscope $end\n");
	fprintf(state.vcdfile, "$enddefinitions $end\n");

	atexit(gpio_exit);

	done_setup = true;
}
