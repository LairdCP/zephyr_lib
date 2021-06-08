/**
 * @file lcz_bt_security.h
 * @brief Although the IO capabilities of a device are fixed, the Zephyr
 * implementation isn't ideal for multiple connections because it adds coupling.
 *
 * Wrap the security callback so that it can be used by independent modules
 * (based on connection).
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_BT_SECURITY_H__
#define __LCZ_BT_SECURITY_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>
#include <stddef.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Register security (pairing) callbacks.
 *
 * @param cb callback struct @ref conn.h
 * @return int negative error code, 0 on success
 */
int lcz_bt_security_register_cb(const struct bt_conn_auth_cb *cb);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_BT_SECURITY_H__ */
