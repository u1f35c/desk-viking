// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Various utility functions
 *
 * Copyright 2021 Jonathan McDowell <noodles@earth.li>
 */

#include "util.h"

/**
 * Take a 4 bit nibble and return its ASCII hex representation.
 *
 * :param x: The nibble to represent. Will be masked to its lower 4 bits.
 * :return: The ASCII character representing this nibble
 */
char util_hexchar(uint8_t x)
{
	x &= 0xF;

	if (x <= 9)
		return '0' + x;
	else if (x <= 0xF)
		return 'A' + x - 10;
	else
		return '?';
}
