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
#include <tinycbor/cbor.h>
#include <tinycbor/cbor_buf_writer.h>
#include "cborattr/cborattr.h"
#include "mgmt/mgmt.h"
#include <shell/shell.h>
#include <shell/shell_uart.h>

#include "shell_log_mgmt.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define NOT_A_BOOL -1

#define END_OF_CBOR_ATTR_ARRAY                                                 \
	{                                                                      \
		.attribute = NULL                                              \
	}

#define MGMT_STATUS_CHECK(x) ((x != 0) ? MGMT_ERR_ENOMEM : 0)

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static mgmt_handler_fn uart_log_halt;

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
	CborError err = 0;
	/* Use an integer to check if the boolean type was found. */
	union {
		long long int integer;
		bool boolean;
	} value;
	value.integer = NOT_A_BOOL;
	int r = -EPERM;

	struct cbor_attr_t params_attr[] = {
		{
			.attribute = "p1",
			.type = CborAttrBooleanType,
			.addr.boolean = &value.boolean,
			.nodefault = true,
		},
		END_OF_CBOR_ATTR_ARRAY
	};

	if (cbor_read_object(&ctxt->it, params_attr) != 0) {
		return MGMT_ERR_EINVAL;
	}

	if (value.integer == NOT_A_BOOL) {
		return MGMT_ERR_EINVAL;
	}

	if (value.boolean) {
		r = shell_execute_cmd(shell_backend_uart_get_ptr(), "log halt");
	} else {
		r = shell_execute_cmd(shell_backend_uart_get_ptr(), "log go");
	}

	err |= cbor_encode_text_stringz(&ctxt->encoder, "r");
	err |= cbor_encode_int(&ctxt->encoder, r);

	return MGMT_STATUS_CHECK(err);
#else
	ARG_UNUSED(ctxt);

	return MGMT_ERR_ENOTSUP;
#endif
}
