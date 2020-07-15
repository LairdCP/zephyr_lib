/**
 * @file fota.h
 * @brief Firmware update over-the-air BLE service.
 * This services works in conjuction with mcumgr upload/download and a
 * file system.
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __FOTA_H__
#define __FOTA_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <bluetooth/conn.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Constants, Macros and Type Definitions                              */
/******************************************************************************/

/* clang-format off */
#define FOTA_NOP            0
#define FOTA_LIST_FILES     1
#define FOTA_MODEM_START    2
#define FOTA_DELETE_FILES   3
#define FOTA_COMPUTE_SHA256 4

#define FOTA_STATUS_SUCCESS 0
#define FOTA_STATUS_BUSY    1
#define FOTA_STATUS_ERROR   2
/* clang-format on */

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief For multi-peripheral device the weak implementation can be overriden.
 */
struct bt_conn *fota_get_conn(void);

/**
 * @brief Initialize and register the FOTA service.
 */
void fota_init();

/**
 * @brief Sets status in service and, if enabled, sends notification.
 */
void fota_set_status(int status);

/**
 * @brief Sets byte count in service and, if enabled, sends notification.
 */
void fota_set_count(uint32_t count);

/**
 * @brief Sets file size in service and, if enabled, sends notification.
 */
void fota_set_size(uint32_t size);

/**
 * @brief Sets name in service and, if enabled, sends notification.
 */
void fota_set_file_name(const char *name);

/**
 * @brief Sets hash in service and, if enabled, sends notification.
 */
void fota_set_hash(const uint8_t *hash);

/**
 * @brief Sets state in service and, if enabled, sends notification.
 * @note When busy the FOTA service will not accept another command.
 */
void fota_state_handler(uint8_t state);

#ifdef __cplusplus
}
#endif

#endif /* __FOTA_H__ */
