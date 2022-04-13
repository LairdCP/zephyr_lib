/**
 * @file qrtc_mgmt.c
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

#include "attr.h"
#include "lcz_qrtc.h"
#include "qrtc_mgmt.h"

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static int set_rtc(struct mgmt_ctxt *ctxt);
static int get_rtc(struct mgmt_ctxt *ctxt);

static int qrtc_mgmt_init(const struct device *device);

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static const struct mgmt_handler QRTC_MGMT_HANDLERS[] = {
	[QRTC_MGMT_ID_SET_RTC] = {
		.mh_write = set_rtc,
		.mh_read = NULL
	},
	[QRTC_MGMT_ID_GET_RTC] = {
		.mh_write = NULL,
		.mh_read = get_rtc
	}
};

static struct mgmt_group qrtc_mgmt_group = {
	.mg_handlers = QRTC_MGMT_HANDLERS,
	.mg_handlers_count = QRTC_MGMT_HANDLER_CNT,
	.mg_group_id = CONFIG_MGMT_GROUP_ID_QRTC,
};

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
SYS_INIT(qrtc_mgmt_init, APPLICATION, 99);

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static int qrtc_mgmt_init(const struct device *device)
{
	ARG_UNUSED(device);

	mgmt_register_group(&qrtc_mgmt_group);

	return 0;
}

static int set_rtc(struct mgmt_ctxt *ctxt)
{
	int r = 0;
	int t = 0;
	uint32_t epoch = ULONG_MAX;
	zcbor_state_t *zse = ctxt->cnbe->zs;
	zcbor_state_t *zsd = ctxt->cnbd->zs;
	size_t decoded;
	bool ok;

#ifdef CONFIG_ATTR_SETTINGS_LOCK
	if (attr_is_locked() == true) {
		r = -EACCES;
		t = lcz_qrtc_get_epoch();
	} else {
#endif

		struct zcbor_map_decode_key_val rtc_set_rtc_decode[] = {
			ZCBOR_MAP_DECODE_KEY_VAL(p1, zcbor_uint32_decode,
						 &epoch),
		};


		ok = zcbor_map_decode_bulk(zsd, rtc_set_rtc_decode,
					   ARRAY_SIZE(rtc_set_rtc_decode),
					   &decoded) == 0;

		if (!ok || decoded == 0) {
			return MGMT_ERR_EINVAL;
		}

		r = attr_set_uint32(ATTR_ID_qrtc_last_set, epoch);
		t = lcz_qrtc_set_epoch(epoch);

#ifdef CONFIG_ATTR_SETTINGS_LOCK
	}
#endif

	/* Cbor encode result */
	ok = zcbor_tstr_put_lit(zse, "r")	&&
	     zcbor_int32_put(zse, r)		&&
	     zcbor_tstr_put_lit(zse, "t")	&&
	     zcbor_uint32_put(zse, t);

	/* Exit with result */
	return ok ? MGMT_ERR_EOK : MGMT_ERR_ENOMEM;
}

static int get_rtc(struct mgmt_ctxt *ctxt)
{
	zcbor_state_t *zse = ctxt->cnbe->zs;
	bool ok;

	/* Cbor encode result */
	ok = zcbor_tstr_put_lit(zse, "t")	&&
	     zcbor_uint32_put(zse, lcz_qrtc_get_epoch());

	/* Exit with result */
	return ok ? MGMT_ERR_EOK : MGMT_ERR_ENOMEM;
}
