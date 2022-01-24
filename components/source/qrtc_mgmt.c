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
#include <tinycbor/cbor.h>
#include <tinycbor/cbor_buf_writer.h>
#include "cborattr/cborattr.h"
#include "mgmt/mgmt.h"

#include "attr.h"
#include "lcz_qrtc.h"
#include "qrtc_mgmt.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define END_OF_CBOR_ATTR_ARRAY                                                 \
	{                                                                      \
		.attribute = NULL                                              \
	}

#define MGMT_STATUS_CHECK(x) ((x != 0) ? MGMT_ERR_ENOMEM : 0)

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static mgmt_handler_fn set_rtc;
static mgmt_handler_fn get_rtc;

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
	CborError err = 0;
	int r = 0;
	int t = 0;
	long long unsigned int epoch = ULLONG_MAX;

	struct cbor_attr_t params_attr[] = {
		{ .attribute = "p1",
		  .type = CborAttrUnsignedIntegerType,
		  .addr.uinteger = &epoch,
		  .nodefault = true },
		END_OF_CBOR_ATTR_ARRAY
	};

	if (cbor_read_object(&ctxt->it, params_attr) != 0) {
		return MGMT_ERR_EINVAL;
	}

#ifdef CONFIG_ATTR_SETTINGS_LOCK
	if (attr_is_locked() == true) {
		r = -EACCES;
		t = lcz_qrtc_get_epoch();
	}
#endif

	if (r == 0 && epoch < UINT32_MAX) {
		r = attr_set_uint32(ATTR_ID_qrtc_last_set, epoch);
		t = lcz_qrtc_set_epoch(epoch);
	} else if (r == 0 && epoch >= UINT32_MAX) {
		r = -EINVAL;
		t = lcz_qrtc_get_epoch();
	}

	err |= cbor_encode_text_stringz(&ctxt->encoder, "r");
	err |= cbor_encode_int(&ctxt->encoder, r);
	err |= cbor_encode_text_stringz(&ctxt->encoder, "t");
	err |= cbor_encode_int(&ctxt->encoder, t);

	return MGMT_STATUS_CHECK(err);
}

static int get_rtc(struct mgmt_ctxt *ctxt)
{
	int t = lcz_qrtc_get_epoch();
	CborError err = 0;

	err |= cbor_encode_text_stringz(&ctxt->encoder, "t");
	err |= cbor_encode_int(&ctxt->encoder, t);

	return MGMT_STATUS_CHECK(err);
}
