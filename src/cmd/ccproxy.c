// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * CCLib compatible ccproxy implementation
 *
 * See https://github.com/wavesoft/CCLib
 *
 * Copyright 2021 Jonathan McDowell <noodles@earth.li>
 */
#include <string.h>

#include "cdc.h"
#include "ccdbg.h"
#include "debug.h"
#include "gpio.h"

/* Commands, as per CCLib */
#define CMD_ENTER	0x01
#define CMD_EXIT	0x02
#define CMD_CHIP_ID	0x03
#define CMD_STATUS	0x04
#define CMD_PC		0x05
#define CMD_STEP	0x06
#define CMD_EXEC_1	0x07
#define CMD_EXEC_2	0x08
#define CMD_EXEC_3	0x09
#define CMD_BURSTWR	0x0A
#define CMD_RD_CFG	0x0B
#define CMD_WR_CFG	0x0C
#define CMD_CHPERASE	0x0D
#define CMD_RESUME	0x0E
#define CMD_HALT	0x0F
#define CMD_PING	0xF0
#define CMD_INSTR_VER	0xF1
#define CMD_INSTR_UPD	0xF2

/* Response codes, as per CCLib */
#define ANS_OK		1
#define ANS_ERROR	2
#define ANS_READY	3

void ccproxy_sendframe(struct cdc *tty, uint8_t ans, uint8_t b0, uint8_t b1)
{
	char buf[3];

	buf[0] = ans;
	buf[1] = b1;
	buf[2] = b0;
	cdc_send(tty, buf, 3);
}

static void ccproxy_sendresp(struct cdc *tty, struct ccdbg_state *ctx,
		uint8_t b0, uint8_t b1)
{
	uint8_t status;

	status = ccdbg_error(ctx);
	if (status)
		ccproxy_sendframe(tty, ANS_ERROR, status, 0);
	else
		ccproxy_sendframe(tty, ANS_OK, b0, b1);
}

static void ccproxy_handle_cmd(struct cdc *tty, struct ccdbg_state *ctx,
		char *cmd)
{
	int read;
	uint8_t ret;
	uint8_t left;
	uint16_t status;

	switch (cmd[0]) {
	case CMD_PING:
		debug_print("CCProxy: Ping\r\n");
		ccproxy_sendframe(tty, ANS_OK, 0, 0);
		break;
	case CMD_ENTER:
		debug_print("CCProxy: Enter\r\n");
		ccdbg_enter(ctx);
		ccproxy_sendresp(tty, ctx, 0, 0);
		break;
	case CMD_EXIT:
		debug_print("CCProxy: Exit\r\n");
		ccdbg_exit(ctx);
		ccproxy_sendresp(tty, ctx, 0, 0);
		break;
	case CMD_CHIP_ID:
		debug_print("CCProxy: CHIP ID\r\n");
		status = ccdbg_chipid(ctx);
		ccproxy_sendresp(tty, ctx, status & 0xFF,
				(status >> 8) & 0xFF);
		break;
	case CMD_STATUS:
		debug_print("CCProxy: STATUS\r\n");
		ret = ccdbg_status(ctx);
		ccproxy_sendresp(tty, ctx, ret, 0);
		break;
	case CMD_PC:
		status = ccdbg_getPC(ctx);
		ccproxy_sendresp(tty, ctx, status & 0xFF,
				(status >> 8) & 0xFF);
		break;
	case CMD_STEP:
		ret = ccdbg_step(ctx);
		ccproxy_sendresp(tty, ctx, ret, 0);
		break;
	case CMD_EXEC_1:
		ret = ccdbg_exec1(ctx, cmd[1]);
		ccproxy_sendresp(tty, ctx, ret, 0);
		break;
	case CMD_EXEC_2:
		ret = ccdbg_exec2(ctx, cmd[1], cmd[2]);
		ccproxy_sendresp(tty, ctx, ret, 0);
		break;
	case CMD_EXEC_3:
		ret = ccdbg_exec3(ctx, cmd[1], cmd[2], cmd[3]);
		ccproxy_sendresp(tty, ctx, ret, 0);
		break;
	case CMD_RD_CFG:
		debug_print("CCProxy: READ CONFIG\r\n");
		ret = ccdbg_readcfg(ctx);
		ccproxy_sendresp(tty, ctx, ret, 0);
		break;
	case CMD_WR_CFG:
		ret = ccdbg_writecfg(ctx, cmd[1]);
		ccproxy_sendresp(tty, ctx, ret, 0);
		break;
	case CMD_CHPERASE:
		ret = ccdbg_chiperase(ctx);
		ccproxy_sendresp(tty, ctx, ret, 0);
		break;
	case CMD_RESUME:
		ret = ccdbg_resume(ctx);
		ccproxy_sendresp(tty, ctx, ret, 0);
		break;
	case CMD_HALT:
		ret = ccdbg_halt(ctx);
		ccproxy_sendresp(tty, ctx, ret, 0);
		break;
	case CMD_INSTR_VER:
		ret = ccdbg_instrtblver(ctx);
		ccproxy_sendresp(tty, ctx, ret, 0);
		break;
	case CMD_INSTR_UPD:
		debug_print("CCProxy: INSTR_UPD\r\n");
		ccproxy_sendframe(tty, ANS_READY, 0, 0);

		/* Read the next 16 bytes into our instruction table */
		ret = 0;
		left = CCDBG_INSTRLEN;
		while (left > 0 && (read = cdc_recv(tty, cmd, NULL)) > 0) {
			if (read > left)
				read = left;

			ret = ccdbg_updateinstr(ctx, cmd,
					CCDBG_INSTRLEN - left, read);

			left -= read;
		}

		/* Indicate success and return the new version */
		ccproxy_sendframe(tty, ANS_OK, ret, 0);
		break;
	case CMD_BURSTWR:
	default:
		debug_print("CCProxy: Error\r\n");
		ccproxy_sendframe(tty, ANS_ERROR, 0xFF, 0);
	}
}

void ccproxy_main(struct cdc *tty, const char *s, int len)
{
	char buf[64];
	int cur_len, i, ret;
	struct ccdbg_state *ctx;

	memcpy(buf, s, len);
	cur_len = len;

	ctx = ccdbg_init(PIN_AUX, PIN_CLK, PIN_MOSI);

	while (1) {
		i = 0;
		while (cur_len >= 4) {
			ccproxy_handle_cmd(tty, ctx, &buf[i]);
			i += 4;
			cur_len -= 4;
		}
		/*
		 * Cope with the situation where we get a full command and part
		 * of the next one. Should be unlikely; we expect a single full
		 * command at a time.
		 */
		if (cur_len > 0)
			memmove(buf, &buf[i], cur_len);

		ret = cdc_recv(tty, &buf[cur_len], NULL);
		if (ret < 0)
			break;
		cur_len += ret;
	}

	/* Disconnected or error, return to main command loop */
}
