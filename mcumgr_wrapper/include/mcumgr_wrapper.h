/**
 * @file mcumgr_wrapper.h
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __MCUMGR_H__
#define __MCUMGR_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Register the commands/subsystems for use with mcumgr.
 *
 * @note If CONFIG_SIMPLE_BLUETOOTH is enabled then the device will
 * advertise the SMP service used by MCUMGR.  This is only needed for devices
 * that want to test BT+MCUMGR that don't already have Bluetooth.
 */
void mcumgr_wrapper_register_subsystems(void);

#ifdef __cplusplus
}
#endif

#endif /* __MCUMGR_H__ */
