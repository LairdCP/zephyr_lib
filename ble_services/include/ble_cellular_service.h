/**
 * @file ble_cellular_service.h
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BLE_CELLULAR_SERVICE_H__
#define __BLE_CELLULAR_SERVICE_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <drivers/modem/hl7800.h>
#include <bluetooth/conn.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/* For multi-peripheral device the weak implementation can be overriden. */
struct bt_conn *cell_svc_get_conn(void);

void cell_svc_init();
void cell_svc_set_imei(const char *imei);
void cell_svc_set_network_state(uint8_t state);
void cell_svc_set_startup_state(uint8_t state);
void cell_svc_set_sleep_state(uint8_t state);
void cell_svc_set_apn(struct mdm_hl7800_apn *access_point);
void cell_svc_set_rssi(int value);
void cell_svc_set_sinr(int value);
void cell_svc_set_fw_ver(const char *ver);
void cell_svc_set_rat(uint8_t value);
void cell_svc_set_iccid(const char *value);
void cell_svc_set_serial_number(const char *value);
void cell_svc_set_bands(char *value);
void cell_svc_set_active_bands(char *value);

#ifdef __cplusplus
}
#endif

#endif /* __BLE_CELLULAR_SERVICE_H__ */
