/**
 * @file memfault_mgmt.c
 * @brief
 *
 * Copyright (c) 2021-2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/zephyr.h>
#include <zephyr/init.h>
#include <limits.h>
#include <string.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <zcbor_bulk/zcbor_bulk_priv.h>
#include <mgmt/mgmt.h>

#include "lcz_memfault.h"
#include "memfault_mgmt.h"

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static int generate_memfault_file(struct mgmt_ctxt *ctxt);

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
	size_t file_size = 0;
	bool has_core_dump = false;
	zcbor_state_t *zse = ctxt->cnbe->zs;
	bool ok;
	int r = lcz_memfault_save_data_to_file(
		CONFIG_MEMFAULT_MGMT_MEMFAULT_FILE_NAME, &file_size,
		&has_core_dump);

	/* Cbor encode result */
	ok = zcbor_tstr_put_lit(zse, "r")					&&
	     zcbor_int32_put(zse, r)						&&
	     zcbor_tstr_put_lit(zse, "s")					&&
	     zcbor_int32_put(zse, file_size)					&&
	     zcbor_tstr_put_lit(zse, "c")					&&
	     zcbor_bool_put(zse, has_core_dump)					&&
	     zcbor_tstr_put_lit(zse, "f")					&&
	     zcbor_tstr_put_term(zse, CONFIG_MEMFAULT_MGMT_MEMFAULT_FILE_NAME)	&&
	     zcbor_tstr_put_lit(zse, "k")					&&
	     zcbor_tstr_put_term(zse, CONFIG_MEMFAULT_NCS_PROJECT_KEY);

	/* Exit with result */
	return ok ? MGMT_ERR_EOK : MGMT_ERR_ENOMEM;
}
#else
static int generate_memfault_file(struct mgmt_ctxt *ctxt)
{
	ARG_UNUSED(ctxt);

	return MGMT_ERR_ENOTSUP;
}
#endif
