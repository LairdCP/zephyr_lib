/**
 * @file ble_power_service.h
 * @brief Allows voltage to be read/notified and reset to be issued.
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BLE_POWER_SERVICE_H__
#define __BLE_POWER_SERVICE_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <bluetooth/conn.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
/* For multi-peripheral device the weak implementation can be overriden. */
struct bt_conn *power_svc_get_conn(void);

void power_svc_init();
void power_svc_set_voltage(uint8_t integer, uint8_t decimal);

#ifdef __cplusplus
}
#endif

#endif /* __BLE_POWER_SERVICE_H__ */