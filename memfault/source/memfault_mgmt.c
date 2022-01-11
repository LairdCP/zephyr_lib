#/**
 * @file memfault_mgmt.c
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

#include "lcz_memfault.h"
#include "memfault_mgmt.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define MGMT_STATUS_CHECK(x) ((x != 0) ? MGMT_ERR_ENOMEM : 0)

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static mgmt_handler_fn generate_memfault_file;

static int memfault_mgmt_init(const struct device *device);

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static const struct mgmt_handler MEMFAULT_MGMT_HANDLERS[] = {
	[MEMFAULT_MGMT_ID_GENERATE_MEMFAULT_FILE] = {
		.mh_write = generate_memfault_file,
		.mh_read = NULL
	}

};

static struct mgmt_group memfault_mgmt_group = {
	.mg_handlers = MEMFAULT_MGMT_HANDLERS,
	.mg_handlers_count = MEMFAULT_MGMT_HANDLER_CNT,
	.mg_group_id = CONFIG_MGMT_GROUP_ID_MEMFAULT,
};

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
SYS_INIT(memfault_mgmt_init, APPLICATION, 99);

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static int memfault_mgmt_init(const struct device *device)
{
	ARG_UNUSED(device);

	mgmt_register_group(&memfault_mgmt_group);

	return 0;
}

#ifdef CONFIG_LCZ_MEMFAULT_FILE
static int generate_memfault_file(struct mgmt_ctxt *ctxt)
{
	CborError err = 0;
	size_t file_size = 0;
	bool has_core_dump = false;
	int r = lcz_memfault_save_data_to_file(
		CONFIG_MEMFAULT_MGMT_MEMFAULT_FILE_NAME, &file_size,
		&has_core_dump);

	err |= cbor_encode_text_stringz(&ctxt->encoder, "r");
	err |= cbor_encode_int(&ctxt->encoder, r);
	err |= cbor_encode_text_stringz(&ctxt->encoder, "s");
	err |= cbor_encode_int(&ctxt->encoder, file_size);
	err |= cbor_encode_text_stringz(&ctxt->encoder, "c");
	err |= cbor_encode_boolean(&ctxt->encoder, has_core_dump);
	err |= cbor_encode_text_stringz(&ctxt->encoder, "f");
	err |= cbor_encode_text_string(
		&ctxt->encoder, CONFIG_MEMFAULT_MGMT_MEMFAULT_FILE_NAME,
		strlen(CONFIG_MEMFAULT_MGMT_MEMFAULT_FILE_NAME));
	err |= cbor_encode_text_stringz(&ctxt->encoder, "k");
	err |= cbor_encode_text_string(&ctxt->encoder,
				       CONFIG_MEMFAULT_NCS_PROJECT_KEY,
				       strlen(CONFIG_MEMFAULT_NCS_PROJECT_KEY));

	return MGMT_STATUS_CHECK(err);
}
#else
static int generate_memfault_file(struct mgmt_ctxt *ctxt)
{
	ARG_UNUSED(ctxt);

	return MGMT_ERR_ENOTSUP;
}
#endif
