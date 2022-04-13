/**
 * @file shell_log_mgmt.c
 * @brief
 *
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr.h>
#include <init.h>
#include <limits.h>
#include <string.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <zcbor_bulk/zcbor_bulk_priv.h>
#include <mgmt/mgmt.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>

#include "shell_log_mgmt.h"

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static int uart_log_halt(struct mgmt_ctxt *ctxt);

static int shell_log_mgmt_init(const struct device *device);

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static const struct mgmt_handler SHELL_LOG_MGMT_HANDLERS[] = {
	[SHELL_LOG_MGMT_ID_UART_LOG_HALT] = {
		.mh_write = uart_log_halt,
		.mh_read = NULL
	}
};

static struct mgmt_group shell_log_mgmt_group = {
	.mg_handlers = SHELL_LOG_MGMT_HANDLERS,
	.mg_handlers_count = SHELL_LOG_MGMT_HANDLER_CNT,
	.mg_group_id = CONFIG_MGMT_GROUP_ID_SHELL_LOG,
};

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
SYS_INIT(shell_log_mgmt_init, APPLICATION, 99);

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static int shell_log_mgmt_init(const struct device *device)
{
	ARG_UNUSED(device);

	mgmt_register_group(&shell_log_mgmt_group);

	return 0;
}

static int uart_log_halt(struct mgmt_ctxt *ctxt)
{
#ifdef CONFIG_SHELL_BACKEND_SERIAL
	/* Use an integer to check if the boolean type was found. */
	int r = -EPERM;
	zcbor_state_t *zse = ctxt->cnbe->zs;
	zcbor_state_t *zsd = ctxt->cnbd->zs;
	size_t decoded;
	bool ok;
	bool halt_log;

	struct zcbor_map_decode_key_val shell_uart_log_halt_decode[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(p1, zcbor_bool_decode, &halt_log),
	};

	ok = zcbor_map_decode_bulk(zsd, shell_uart_log_halt_decode,
		ARRAY_SIZE(shell_uart_log_halt_decode), &decoded) == 0;

	if (!ok || decoded == 0) {
		return MGMT_ERR_EINVAL;
	}

	r = shell_execute_cmd(shell_backend_uart_get_ptr(),
			      (halt_log == true ? "log halt" : "log go"));

	ok = zcbor_tstr_put_lit(zse, "r")	&&
	     zcbor_int32_put(zse, r);

	return ok ? MGMT_ERR_EOK : MGMT_ERR_ENOMEM;
#else
	ARG_UNUSED(ctxt);

	return MGMT_ERR_ENOTSUP;
#endif
}
