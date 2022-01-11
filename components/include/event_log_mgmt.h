/**
 * @file event_log_mgmt.h
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __EVENT_LOG_MGMT_H__
#define __EVENT_LOG_MGMT_H__

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
 * Command IDs for event log management group.
 *
 * @note IDs cannot be changed or re-ordered once set and all handlers must
 * exist in all products, even if they are not used, if a handler is not
 * available for a particular product and/or configuration then it should
 * return MGMT_ERR_ENOTSUP
 */
typedef enum {
	EVENT_LOG_MGMT_ID_PREPARE_LOG,
	EVENT_LOG_MGMT_ID_ACK_LOG,
	EVENT_LOG_MGMT_ID_GENERATE_TEST_LOG
} EVENT_LOG_MGMT_id_t;

#define EVENT_LOG_MGMT_HANDLER_CNT                                              \
	(sizeof EVENT_LOG_MGMT_HANDLERS / sizeof EVENT_LOG_MGMT_HANDLERS[0])

#ifdef __cplusplus
}
#endif

#endif /* __EVENT_LOG_MGMT_H__ */
