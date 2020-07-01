/**
 * @file simple_bluetooth.h
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __SIMPLE_BLUETOOTH_H__
#define __SIMPLE_BLUETOOTH_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Enable Bluetooth and start advertising SMP service.
 */
void simple_bluetooth_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __SIMPLE_BLUETOOTH_H__ */
