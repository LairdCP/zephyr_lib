/**
 * @file shell_log_mgmt.h
 * @brief
 *
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SHELL_LOG_MGMT_H__
#define __SHELL_LOG_MGMT_H__

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
 * Command IDs for shell log management group.
 *
 * @note IDs cannot be changed or re-ordered once set and all handlers must
 * exist in all products, even if they are not used, if a handler is not
 * available for a particular product and/or configuration then it should
 * return MGMT_ERR_ENOTSUP
 */
typedef enum {
	SHELL_LOG_MGMT_ID_UART_LOG_HALT
} SHELL_LOG_MGMT_id_t;

#define SHELL_LOG_MGMT_HANDLER_CNT                                             \
	(sizeof SHELL_LOG_MGMT_HANDLERS / sizeof SHELL_LOG_MGMT_HANDLERS[0])

#ifdef __cplusplus
}
#endif

#endif /* __SHELL_LOG_MGMT_H__ */
