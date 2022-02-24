/**
 * @file event_log_mgmt.c
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
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
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <mgmt/mcumgr/smp_bt.h>
#include <bluetooth/services/dfu_smp.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>

#include "event_log_mgmt.h"
#include "lcz_sensor_event.h"
#include "lcz_event_manager.h"

#ifdef CONFIG_ATTR_SETTINGS_LOCK
#include "attr.h"
#endif

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define MGMT_STATUS_CHECK(x) ((x != 0) ? MGMT_ERR_ENOMEM : 0)

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static int prepare_log(struct mgmt_ctxt *ctxt);
static int ack_log(struct mgmt_ctxt *ctxt);
static int generate_test_log(struct mgmt_ctxt *ctxt);

static int event_log_mgmt_init(const struct device *device);

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static const struct mgmt_handler EVENT_LOG_MGMT_HANDLERS[] = {
	[EVENT_LOG_MGMT_ID_PREPARE_LOG] = {
		.mh_write = prepare_log,
		.mh_read = NULL,
	},
	[EVENT_LOG_MGMT_ID_ACK_LOG] = {
		.mh_write = ack_log,
		.mh_read = NULL,
	},
	[EVENT_LOG_MGMT_ID_GENERATE_TEST_LOG] = {
		.mh_write = generate_test_log,
		.mh_read = NULL
	}
};

static struct mgmt_group event_log_mgmt_group = {
	.mg_handlers = EVENT_LOG_MGMT_HANDLERS,
	.mg_handlers_count = EVENT_LOG_MGMT_HANDLER_CNT,
	.mg_group_id = CONFIG_MGMT_GROUP_ID_EVENT_LOG,
};

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
SYS_INIT(event_log_mgmt_init, APPLICATION, 99);

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static int event_log_mgmt_init(const struct device *device)
{
	ARG_UNUSED(device);

	mgmt_register_group(&event_log_mgmt_group);

	return 0;
}

static int prepare_log(struct mgmt_ctxt *ctxt)
{
	uint8_t n[LCZ_EVENT_MANAGER_FILENAME_SIZE];
	int r = 0;
	uint32_t s = 0;

#ifdef CONFIG_ATTR_SETTINGS_LOCK
	if (attr_is_locked() == true) {
		r = -EPERM;
	}
#endif

	if (r == 0) {
		/* Check if we can prepare the log file OK */
		r = lcz_event_manager_prepare_log_file(n, &s);
		if (r != 0) {
			/* If not, blank the file path */
			n[0] = 0;
		}
	}

	/* Cbor encode result */
	CborError err = 0;

	/* Add result of log prepare */
	err |= cbor_encode_text_stringz(&ctxt->encoder, "r");
	err |= cbor_encode_int(&ctxt->encoder, r);

	/* Add the file size */
	err |= cbor_encode_text_stringz(&ctxt->encoder, "s");
	err |= cbor_encode_int(&ctxt->encoder, s);

	/* Add file path */
	err |= cbor_encode_text_stringz(&ctxt->encoder, "n");
	err |= cbor_encode_text_string(&ctxt->encoder, n, strlen(n));

	/* Exit with result */
	return MGMT_STATUS_CHECK(err);
}

static int ack_log(struct mgmt_ctxt *ctxt)
{
	int r = 0;

#ifdef CONFIG_ATTR_SETTINGS_LOCK
	if (attr_is_locked() == true) {
		r = -EPERM;
	}
#endif

	if (r == 0) {
		r = lcz_event_manager_delete_log_file();
	}

	/* Cbor encode result */
	CborError err = 0;

	/* Add result of log delete */
	err |= cbor_encode_text_stringz(&ctxt->encoder, "r");
	err |= cbor_encode_int(&ctxt->encoder, r);

	/* Exit with result */
	return MGMT_STATUS_CHECK(err);
}

static int generate_test_log(struct mgmt_ctxt *ctxt)
{
	uint8_t n[LCZ_EVENT_MANAGER_FILENAME_SIZE];
	uint32_t s = 0;
	int r = 0;
	long long unsigned int start_time_stamp = 0;
	long long unsigned int update_rate = 0;
	long long unsigned int event_type = 0;
	long long unsigned int event_count = 0;
	long long unsigned int event_data_type = 0;
	DummyLogFileProperties_t dummy_log_file_properties;

	struct cbor_attr_t params_attr[] = {
		{ .attribute = "p1",
		  .type = CborAttrUnsignedIntegerType,
		  .addr.uinteger = &start_time_stamp,
		  .nodefault = true },
		{ .attribute = "p2",
		  .type = CborAttrUnsignedIntegerType,
		  .addr.uinteger = &update_rate,
		  .nodefault = true },
		{ .attribute = "p3",
		  .type = CborAttrUnsignedIntegerType,
		  .addr.uinteger = &event_type,
		  .nodefault = true },
		{ .attribute = "p4",
		  .type = CborAttrUnsignedIntegerType,
		  .addr.uinteger = &event_count,
		  .nodefault = true },
		{ .attribute = "p5",
		  .type = CborAttrUnsignedIntegerType,
		  .addr.uinteger = &event_data_type,
		  .nodefault = true },
		{ .attribute = NULL }
	};

#ifdef CONFIG_ATTR_SETTINGS_LOCK
	if (attr_is_locked() == true) {
		r = -EPERM;
	}
#endif

	if (r == 0) {
		if (cbor_read_object(&ctxt->it, params_attr) != 0) {
			return -EINVAL;
		}

		dummy_log_file_properties.start_time_stamp =
			((uint32_t)(start_time_stamp));
		dummy_log_file_properties.update_rate =
			((uint32_t)(update_rate));
		dummy_log_file_properties.event_type =
			((uint8_t)(event_type));
		dummy_log_file_properties.event_count =
			((uint32_t)(event_count));
		dummy_log_file_properties.event_data_type =
			((uint8_t)(event_data_type));

		/* Check if we can prepare the log file OK */
		r = lcz_event_manager_prepare_test_log_file(
				&dummy_log_file_properties, n, &s);

		if (r != 0) {
			/* If not, blank the file path */
			n[0] = 0;
		}
	}

	/* Cbor encode result */
	CborError err = 0;

	/* Add result of log prepare */
	err |= cbor_encode_text_stringz(&ctxt->encoder, "r");
	err |= cbor_encode_int(&ctxt->encoder, r);

	/* Add the file size */
	err |= cbor_encode_text_stringz(&ctxt->encoder, "s");
	err |= cbor_encode_int(&ctxt->encoder, s);

	/* Add file path */
	err |= cbor_encode_text_stringz(&ctxt->encoder, "n");
	err |= cbor_encode_text_string(&ctxt->encoder, n, strlen(n));

	/* Exit with result */
	return MGMT_STATUS_CHECK(err);
}
