// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * CC.Debugger implementation for Texas Instruments CCxxxx chips to match the
 * CCLib ccproxy implementation
 *
 * See https://github.com/wavesoft/CCLib
 *
 * Copyright 2021 Jonathan McDowell <noodles@earth.li>
 */

#ifndef __CCDBG_H__
#define __CCDBG_H__

#define CCDBG_INSTRLEN	16

struct ccdbg_state;

struct ccdbg_state *ccdbg_init(uint8_t rst, uint8_t dc, uint8_t dd);
void ccdbg_enter(struct ccdbg_state *ctx);
void ccdbg_exit(struct ccdbg_state *ctx);
uint8_t ccdbg_error(struct ccdbg_state *ctx);
uint8_t ccdbg_read(struct ccdbg_state *ctx);
bool ccdbg_write(struct ccdbg_state *ctx, uint8_t b);
uint8_t ccdbg_status(struct ccdbg_state *ctx);
uint8_t ccdbg_step(struct ccdbg_state *ctx);
uint8_t ccdbg_exec1(struct ccdbg_state *ctx, uint8_t c);
uint8_t ccdbg_exec2(struct ccdbg_state *ctx, uint8_t c1, uint8_t c2);
uint8_t ccdbg_exec3(struct ccdbg_state *ctx, uint8_t c1, uint8_t c2,
		uint8_t c3);
uint8_t ccdbg_readcfg(struct ccdbg_state *ctx);
uint8_t ccdbg_writecfg(struct ccdbg_state *ctx, uint8_t c);
uint8_t ccdbg_chiperase(struct ccdbg_state *ctx);
uint8_t ccdbg_resume(struct ccdbg_state *ctx);
uint8_t ccdbg_halt(struct ccdbg_state *ctx);
uint8_t ccdbg_instrtblver(struct ccdbg_state *ctx);
uint8_t ccdbg_updateinstr(struct ccdbg_state *ctx, const uint8_t *buf, int ofs,
		int len);
uint16_t ccdbg_chipid(struct ccdbg_state *ctx);
uint16_t ccdbg_getPC(struct ccdbg_state *ctx);

#endif /* __CCDBG_H__ */
