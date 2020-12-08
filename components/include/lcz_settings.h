/**
 * @file lcz_settings.h
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_SETTINGS_H__
#define __LCZ_SETTINGS_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>
#include <stddef.h>
#include <fs/fs.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Write to the filesystem
 *
 * @param name file name
 * @param data pointer to data
 * @param size size of data
 *
 * @retval negative error code, 0 on success.
 */
ssize_t lcz_settings_write(char *name, void *data, size_t size);

/**
 * @brief Read from the filesystem.
 *
 * @param name file name
 * @param data pointer to data
 * @param size max size of read
 *
 * @retval negative error code, bytes read on success.
 */
ssize_t lcz_settings_read(char *name, void *data, size_t size);

/**
 * @brief Override in application to use a different file system mount
 * point than the one used by the FSU module.
 *
 * @retval negative error code, 0 on success
 */
int lcz_settings_mount_fs(void);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_SETTINGS_H__ */
