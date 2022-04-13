/**
 * @file event_log_mgmt.c
 * @brief
 *
 * Copyright (c) 2021-2022 Laird Connectivity
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
	zcbor_state_t *zse = ctxt->cnbe->zs;
	bool ok;

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
	ok = zcbor_tstr_put_lit(zse, "r")	&&
	     zcbor_int32_put(zse, r)		&&
	     zcbor_tstr_put_lit(zse, "s")	&&
	     zcbor_int32_put(zse, s)		&&
	     zcbor_tstr_put_lit(zse, "n")	&&
	     zcbor_tstr_put_term(zse, n);

	/* Exit with result */
	return ok ? MGMT_ERR_EOK : MGMT_ERR_ENOMEM;
}

static int ack_log(struct mgmt_ctxt *ctxt)
{
	int r = 0;
	zcbor_state_t *zse = ctxt->cnbe->zs;
	bool ok;

#ifdef CONFIG_ATTR_SETTINGS_LOCK
	if (attr_is_locked() == true) {
		r = -EPERM;
	}
#endif

	if (r == 0) {
		r = lcz_event_manager_delete_log_file();
	}

	/* Cbor encode result of log delete */
	ok = zcbor_tstr_put_lit(zse, "r")	&&
	     zcbor_int32_put(zse, r);

	/* Exit with result */
	return ok ? MGMT_ERR_EOK : MGMT_ERR_ENOMEM;
}

static int generate_test_log(struct mgmt_ctxt *ctxt)
{
	uint8_t n[LCZ_EVENT_MANAGER_FILENAME_SIZE];
	uint32_t s = 0;
	int r = 0;
	uint32_t event_type = 0;
	uint32_t event_data_type = 0;
	DummyLogFileProperties_t dummy_log_file_properties = { 0 };
	zcbor_state_t *zse = ctxt->cnbe->zs;
	zcbor_state_t *zsd = ctxt->cnbd->zs;
	size_t decoded;
	bool ok;

#ifdef CONFIG_ATTR_SETTINGS_LOCK
	if (attr_is_locked() == true) {
		r = -EPERM;
	}
#endif

	struct zcbor_map_decode_key_val evt_generate_test_log_decode[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(p1, zcbor_uint32_decode,
					 &dummy_log_file_properties.start_time_stamp),
		ZCBOR_MAP_DECODE_KEY_VAL(p2, zcbor_uint32_decode,
					 &dummy_log_file_properties.update_rate),
		ZCBOR_MAP_DECODE_KEY_VAL(p3, zcbor_uint32_decode, &event_type),
		ZCBOR_MAP_DECODE_KEY_VAL(p4, zcbor_uint32_decode,
					 &dummy_log_file_properties.event_count),
		ZCBOR_MAP_DECODE_KEY_VAL(p5, zcbor_uint32_decode,
					 &event_data_type),
	};

	ok = zcbor_map_decode_bulk(zsd, evt_generate_test_log_decode,
		ARRAY_SIZE(evt_generate_test_log_decode), &decoded) == 0;

	if (!ok) {
		return MGMT_ERR_EINVAL;
	}

	dummy_log_file_properties.event_type = (uint8_t)event_type;
	dummy_log_file_properties.event_data_type = (uint8_t)event_data_type;

	/* Check if we can prepare the log file OK */
	r = lcz_event_manager_prepare_test_log_file(
			&dummy_log_file_properties, n, &s);

	if (r != 0) {
		/* If not, blank the file path */
		n[0] = 0;
	}

	/* Cbor encode result */
	ok = zcbor_tstr_put_lit(zse, "r")	&&
	     zcbor_int32_put(zse, r)		&&
	     zcbor_tstr_put_lit(zse, "s")	&&
	     zcbor_int32_put(zse, s)		&&
	     zcbor_tstr_put_lit(zse, "n")	&&
	     zcbor_tstr_put_term(zse, n);

	/* Exit with result */
	return ok ? MGMT_ERR_EOK : MGMT_ERR_ENOMEM;
}
