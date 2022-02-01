/**
 * @file lcz_ramdisk.h
 * @brief RAMDISK
 *
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __RAMDISK_H__
#define __RAMDISK_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Mount RAMDISK-based lfs partiiton
 *
 * @retval negative error code, 0 on success.
 */
int ramfs_mount(void);

#ifdef __cplusplus
}
#endif

#endif /* __RAMDISK_H__ */
