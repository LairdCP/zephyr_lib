#/**
 * @file memfault_mgmt.h
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MEMFAULT_MGMT_H__
#define __MEMFAULT_MGMT_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
/**
 * Command IDs for memfault management group.
 *
 * @note IDs cannot be changed or re-ordered once set and all handlers must
 * exist in all products, even if they are not used, if a handler is not
 * available for a particular product and/or configuration then it should
 * return MGMT_ERR_ENOTSUP
 */
typedef enum {
	MEMFAULT_MGMT_ID_GENERATE_MEMFAULT_FILE
} MEMFAULT_MGMT_id_t;

#define MEMFAULT_MGMT_HANDLER_CNT                                              \
	(sizeof MEMFAULT_MGMT_HANDLERS / sizeof MEMFAULT_MGMT_HANDLERS[0])

#ifdef __cplusplus
}
#endif

#endif /* __MEMFAULT_MGMT_H__ */
