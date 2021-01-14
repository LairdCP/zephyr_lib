/**
 * @file lcz_bt_scan.h
 * @brief Controls Bluetooth scanning for multiple centrals/observers with a
 * counting semaphore.
 *
 * @note Scanning must be disabled in order to connect.
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_BT_SCAN_H__
#define __LCZ_BT_SCAN_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>
#include <stddef.h>
#include <bluetooth/bluetooth.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Sets scan parameters if number of scan modules users is zero.
 *
 * @param p Bluetooth scan parameters.
 *
 * @retval true if success, false otherwise
 */
bool lcz_bt_scan_set_parameters(const struct bt_le_scan_param *param);

/**
 * @brief Register user of scan module.
 *
 * @param pId user id
 * @param cb Register advertisement handler callback
 *
 * @retval true if new user was registered, false otherwise.
 */
bool lcz_bt_scan_register(int *pId, bt_le_scan_cb_t *cb);

/**
 * @brief Start scanning (if there aren't any stop requests).
 */
void lcz_bt_scan_start(int id);

/**
 * @brief Stop scanning.
 */
void lcz_bt_scan_stop(int id);

/**
 * @brief Clear stop request.
 * Restart scanning if there aren't any stop requests and there is at least
 * one start request.
 */
void lcz_bt_scan_resume(int id);

/**
 * @brief Clear stop request, set start request, and start scanning
 * if there aren't any other stop requests.
 */
void lcz_bt_scan_restart(int id);

/**
 * @brief Accessor function
 *
 * @retval true if scanning, false otherwise
 */
bool lcz_bt_scan_active(void);

/**
 * @brief Accessor function
 *
 * @retval number of scan starts (!= the number of scan start requests)
 */
uint32_t lcz_bt_scan_get_num_starts(void);

/**
 * @brief Accessor function
 *
 * @retval number of scan stops
 */
uint32_t lcz_bt_scan_get_num_stops(void);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_BT_SCAN_H__ */
