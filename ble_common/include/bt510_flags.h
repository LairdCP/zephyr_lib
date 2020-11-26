/**
 * @file bt510_flags.h
 * @brief Advertisement flags field for BT510
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT510_FLAGS_H__
#define __BT510_FLAGS_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Constants, Macros and Type Definitions                              */
/******************************************************************************/
/** mask, position */
/* clang-format off */
#define BT510_FLAG_TIME_WAS_SET              0x1, 0
#define BT510_FLAG_ACTIVE_MODE               0x1, 1
#define BT510_FLAG_ANY_ALARM                 0x1, 2
#define BT510_FLAG_LOW_BATTERY_ALARM         0x1, 7
#define BT510_FLAG_HIGH_TEMP_ALARM           0x3, 8
#define BT510_FLAG_LOW_TEMP_ALARM            0x3, 10
#define BT510_FLAG_DELTA_TEMP_ALARM          0x1, 12
#define BT510_FLAG_RATE_OF_CHANGE_TEMP_ALARM 0x1, 13
#define BT510_FLAG_MOVEMENT_ALARM            0x1, 14
#define BT510_FLAG_MAGNET_STATE              0x1, 15
/* clang-format on */

#define BT510_ANY_ALARM_MASK 0x00007F10

#ifdef __cplusplus
}
#endif

#endif /* __BT510_FLAGS_H__ */
