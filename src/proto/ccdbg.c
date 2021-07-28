// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * CC.Debugger implementation for Texas Instruments CCxxxx chips to match the
 * CCLib ccproxy implementation
 *
 * See https://github.com/wavesoft/CCLib
 *
 * Copyright 2021 Jonathan McDowell <noodles@earth.li>
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <chopstx.h>

#include "ccdbg.h"
#include "gpio.h"

/*
 * Instruction table indices, from CCLib
 */
#define INSTR_VERSION	0
#define I_HALT		1
#define I_RESUME	2
#define I_RD_CONFIG	3
#define I_WR_CONFIG	4
#define I_DEBUG_INSTR_1	5
#define I_DEBUG_INSTR_2	6
#define I_DEBUG_INSTR_3	7
#define I_GET_CHIP_ID	8
#define I_GET_PC	9
#define I_READ_STATUS	10
#define I_STEP_INSTR	11
#define I_CHIP_ERASE	12

/*
 * Error responses, from CCLib
 */
#define CC_ERROR_NONE		0
#define CC_ERROR_NOT_ACTIVE	1
#define CC_ERROR_NOT_DEBUGGING	2
#define CC_ERROR_NOT_WIRED	3

struct ccdbg_state {
	/* Instruction table */
	uint8_t instr[CCDBG_INSTRLEN];

	/* IO pins */
	uint8_t rst;
	uint8_t dc;
	uint8_t dd;

	uint8_t error;
	bool active;
	bool indebug;
};

static uint8_t ccdbg_read_int(struct ccdbg_state *ctx)
{
	uint8_t b, i;

	if (!ctx->active) {
		ctx->error = CC_ERROR_NOT_ACTIVE;
		return 0;
	}

	/*
	 * Read data msb first
	 * clk high, 2 ms, read state, clock low, 2ms
	 */
	gpio_set_input(ctx->dd);
	b = 0;
	for (i = 0; i < 8; i++) {
		/* Toggle clock and shift data in */
		gpio_set(ctx->dc, true);
		chopstx_usec_wait(2);
		b <<= 1;
		if (gpio_get(ctx->dd))
			b |= 1;
		gpio_set(ctx->dc, false);
		chopstx_usec_wait(2);
	}

	return b;
}

bool ccdbg_write(struct ccdbg_state *ctx, uint8_t b)
{
	int i;

	if (!ctx->active) {
		ctx->error = CC_ERROR_NOT_ACTIVE;
		return false;
	}
	if (!ctx->indebug) {
		ctx->error = CC_ERROR_NOT_DEBUGGING;
		return false;
	}

	/*
	 * DD output
	 * clock data out msb first
	 *  clock high, 2ms, clock low, 2ms
	 */
	gpio_set_output(ctx->dd, false);
	for (i = 0; i < 8; i++) {
		if (b & 0x80)
			gpio_set(ctx->dd, true);
		else
			gpio_set(ctx->dd, false);

		/* Toggle clock and shift data */
		gpio_set(ctx->dc, true);
		b <<= 1;
		chopstx_usec_wait(2);
		gpio_set(ctx->dc, false);
		chopstx_usec_wait(2);
	}

	return true;
}

static bool ccdbg_switchread(struct ccdbg_state *ctx)
{
	int i, count;

	if (!ctx->active) {
		ctx->error = CC_ERROR_NOT_ACTIVE;
		return false;
	}
	if (!ctx->indebug) {
		ctx->error = CC_ERROR_NOT_DEBUGGING;
		return false;
	}

	/*
	 * DD input
	 * Wait 2ms
	 * while DD is high
	 *   8: clk high, 2ms, clk low, 2ms
	 *
	 * Limit cycles to 255.
	 */
	gpio_set_input(ctx->dd);
	chopstx_usec_wait(2);
	count = 255;
	while (gpio_get(ctx->dd)) {
		for (i = 0; i < 8; i++) {
			gpio_set(ctx->dc, true);
			chopstx_usec_wait(2);
			gpio_set(ctx->dc, false);
			chopstx_usec_wait(2);
		}
		if (!--count) {
			ctx->error = CC_ERROR_NOT_WIRED;
			ctx->indebug = false;
			return false;
		}
	}

	if (count < 255)
		chopstx_usec_wait(2);

	return true;
}

static bool ccdbg_switchwrite(struct ccdbg_state *ctx)
{
	if (!ctx->active) {
		ctx->error = CC_ERROR_NOT_ACTIVE;
		return false;
	}
	if (!ctx->indebug) {
		ctx->error = CC_ERROR_NOT_DEBUGGING;
		return false;
	}

	/*
	 * Switch DD to output
	 */
	gpio_set_output(ctx->dd, false);

	return true;
}

uint8_t ccdbg_read(struct ccdbg_state *ctx)
{
	uint8_t ret;

	if (!ccdbg_switchread(ctx))
		return 0;
	ret = ccdbg_read_int(ctx);
	ccdbg_switchwrite(ctx);

	return ret;
}

static uint8_t ccdbg_do_cmd(struct ccdbg_state *ctx, int cmd)
{
	if (!ccdbg_write(ctx, ctx->instr[cmd]))
		return 0;

	return ccdbg_read(ctx);
}

void ccdbg_enter(struct ccdbg_state *ctx)
{
	if (!ctx->active) {
		ctx->error = CC_ERROR_NOT_ACTIVE;
		return;
	}

	ctx->error = CC_ERROR_NONE;
	gpio_set(ctx->rst, false);
	chopstx_usec_wait(200);
	gpio_set(ctx->dc, true);
	chopstx_usec_wait(3);
	gpio_set(ctx->dc, false);
	chopstx_usec_wait(3);
	gpio_set(ctx->dc, true);
	chopstx_usec_wait(3);
	gpio_set(ctx->dc, false);
	chopstx_usec_wait(200);
	gpio_set(ctx->rst, true);
	chopstx_usec_wait(200);

	ctx->indebug = true;
}

void ccdbg_exit(struct ccdbg_state *ctx)
{
	ccdbg_do_cmd(ctx, I_RESUME);
	ctx->indebug = false;
}

uint8_t ccdbg_error(struct ccdbg_state *ctx)
{
	return ctx->error;
}

uint8_t ccdbg_chiperase(struct ccdbg_state *ctx)
{
	return ccdbg_do_cmd(ctx, I_CHIP_ERASE);
}

uint8_t ccdbg_halt(struct ccdbg_state *ctx)
{
	return ccdbg_do_cmd(ctx, I_HALT);
}

uint8_t ccdbg_readcfg(struct ccdbg_state *ctx)
{
	return ccdbg_do_cmd(ctx, I_RD_CONFIG);
}

uint8_t ccdbg_resume(struct ccdbg_state *ctx)
{
	return ccdbg_do_cmd(ctx, I_RESUME);
}

uint8_t ccdbg_status(struct ccdbg_state *ctx)
{
	return ccdbg_do_cmd(ctx, I_READ_STATUS);
}

uint8_t ccdbg_step(struct ccdbg_state *ctx)
{
	return ccdbg_do_cmd(ctx, I_STEP_INSTR);
}

uint8_t ccdbg_exec1(struct ccdbg_state *ctx, uint8_t c)
{
	if (!ccdbg_write(ctx, ctx->instr[I_DEBUG_INSTR_1]))
		return 0;
	if (!ccdbg_write(ctx, c))
		return 0;

	return ccdbg_read(ctx);
}

uint8_t ccdbg_exec2(struct ccdbg_state *ctx, uint8_t c1, uint8_t c2)
{
	if (!ccdbg_write(ctx, ctx->instr[I_DEBUG_INSTR_2]))
		return 0;
	if (!ccdbg_write(ctx, c1))
		return 0;
	if (!ccdbg_write(ctx, c2))
		return 0;

	return ccdbg_read(ctx);
}

uint8_t ccdbg_exec3(struct ccdbg_state *ctx, uint8_t c1, uint8_t c2,
		uint8_t c3)
{
	if (!ccdbg_write(ctx, ctx->instr[I_DEBUG_INSTR_3]))
		return 0;
	if (!ccdbg_write(ctx, c1))
		return 0;
	if (!ccdbg_write(ctx, c2))
		return 0;
	if (!ccdbg_write(ctx, c3))
		return 0;

	return ccdbg_read(ctx);
}

uint8_t ccdbg_writecfg(struct ccdbg_state *ctx, uint8_t c)
{
	if (!ccdbg_write(ctx, ctx->instr[I_WR_CONFIG]))
		return 0;
	if (!ccdbg_write(ctx, c))
		return 0;

	return ccdbg_read(ctx);
}

uint8_t ccdbg_instrtblver(struct ccdbg_state *ctx)
{
	return ctx->instr[INSTR_VERSION];
}

uint8_t ccdbg_updateinstr(struct ccdbg_state *ctx, const char *buf, int ofs, int len)
{
	if (ofs >= CCDBG_INSTRLEN)
		return 0;
	if (len > (CCDBG_INSTRLEN - ofs))
		return 0;

	memcpy(&ctx->instr[ofs], buf, len);

	return ctx->instr[INSTR_VERSION];
}

uint16_t ccdbg_chipid(struct ccdbg_state *ctx)
{
	uint16_t chipid;

	if (!ccdbg_write(ctx, ctx->instr[I_GET_CHIP_ID]))
		return 0;

	if (!ccdbg_switchread(ctx))
		return 0;
	chipid = ccdbg_read_int(ctx);
	chipid <<= 8;
	chipid |= ccdbg_read_int(ctx);
	ccdbg_switchwrite(ctx);

	return chipid;
}

uint16_t ccdbg_getPC(struct ccdbg_state *ctx)
{
	uint16_t pc;

	if (!ccdbg_write(ctx, ctx->instr[I_GET_PC]))
		return 0;

	ccdbg_switchread(ctx);
	pc = ccdbg_read_int(ctx);
	pc <<= 8;
	pc |= ccdbg_read_int(ctx);
	ccdbg_switchwrite(ctx);

	return pc;
}

/* Avoid dynamic allocations */
static struct ccdbg_state static_state;

struct ccdbg_state *ccdbg_init(uint8_t rst, uint8_t dc, uint8_t dd)
{
	struct ccdbg_state *ctx = &static_state;

	ctx->rst = rst;
	ctx->dc = dc;
	ctx->dd = dd;

	/*
	 * Set rst/dc to output, dd to input
	 * All low / no pull up
	 */
	gpio_set_output(ctx->rst, false);
	gpio_set_output(ctx->dc, false);
	gpio_set_input(ctx->dd);

	ctx->instr[INSTR_VERSION]    = 1;
	ctx->instr[I_HALT]           = 0x40;
	ctx->instr[I_RESUME]         = 0x48;
	ctx->instr[I_RD_CONFIG]      = 0x20;
	ctx->instr[I_WR_CONFIG]      = 0x18;
	ctx->instr[I_DEBUG_INSTR_1]  = 0x51;
	ctx->instr[I_DEBUG_INSTR_2]  = 0x52;
	ctx->instr[I_DEBUG_INSTR_3]  = 0x53;
	ctx->instr[I_GET_CHIP_ID]    = 0x68;
	ctx->instr[I_GET_PC]         = 0x28;
	ctx->instr[I_READ_STATUS]    = 0x30;
	ctx->instr[I_STEP_INSTR]     = 0x58;
	ctx->instr[I_CHIP_ERASE]     = 0x10;

	ctx->error = CC_ERROR_NONE;
	ctx->active = true;
	ctx->indebug = false;

	return ctx;
}
