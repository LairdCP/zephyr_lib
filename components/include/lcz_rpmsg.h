/**
 * @file lcz_rpmsg.h
 * @brief Remote procedure message passing to another core
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_RPMSG_H__
#define __LCZ_RPMSG_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Constants, Macros and Type Definitions                              */
/******************************************************************************/
#define LCZ_RPMSG_COMPONENT_ALL 0xff

/**
 * @brief RPMSG component message received callback which is implemented in
 *        the user application
 *
 * @param component Component ID of message
 * @param data Received data buffer
 * @param len Size of received data
 * @param src Source address
 * @param handled True if a previous handler has handled this command
 *
 * @retval true if message has been handled, false otherwise.
 */
typedef bool lcz_rpmsg_msg_cb_t(uint8_t component, void *data, size_t len,
				uint32_t src, bool handled);


/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Register user of RPMSG component.
 *
 * @param pId User ID
 * @param component Component ID for messages (0xff to receive all)
 * @param cb Register RPMSG receive handler callback
 *
 * @retval true if new user was registered, false otherwise.
 */
bool lcz_rpmsg_register(int *pId, uint8_t component, lcz_rpmsg_msg_cb_t *cb);

/**
 * @brief Send a message to another core over RPMSG
 *
 * @param component Component ID to send message to
 * @param data The data object/buffer to send to the other core
 * @param len The length of the data to sent
 *
 * @return int negative error code, 0 or greater (data size sent) on success
 */
int lcz_rpmsg_send(uint8_t component, const void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_RPMSG_H__ */
