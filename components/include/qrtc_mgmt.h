/**
 * @file qrtc_mgmt.h
 *
 * @brief SMP interface for QRTC Command Group
 *
 * Copyright (c) 2021-2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __QRTC_MGMT_H__
#define __QRTC_MGMT_H__

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
 * Command IDs for QRTC management group.
 *
 * @note Location zero isn't used and is reserved for future use
 * @note IDs cannot be changed or re-ordered once set and all handlers must
 * exist in all products, even if they are not used, if a handler is not
 * available for a particular product and/or configuration then it should
 * return MGMT_ERR_ENOTSUP
 */
typedef enum {
	QRTC_MGMT_ID_SET_RTC,
	QRTC_MGMT_ID_GET_RTC
} QRTC_MGMT_id_t;

#define QRTC_MGMT_HANDLER_CNT                                                  \
	(sizeof QRTC_MGMT_HANDLERS / sizeof QRTC_MGMT_HANDLERS[0])

#ifdef __cplusplus
}
#endif

#endif /* __QRTC_MGMT_H__ */
