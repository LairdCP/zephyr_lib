/**
 * @file encrypted_file_storage.h
 *
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ENCRYPTED_FILE_STORAGE_H__
#define __ENCRYPTED_FILE_STORAGE_H__

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <zephyr/types.h>
#include <stddef.h>
#include <fs/fs.h>

#include "file_system_utilities.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************/
/* Global Constants, Macros and Type Definitions                                                  */
/**************************************************************************************************/

/**************************************************************************************************/
/* Global Function Prototypes                                                                     */
/**************************************************************************************************/

/** @brief Write entire contents of encrypted file
 *
 * @param abs_path directory path and name
 * @param data to be written
 * @param size in bytes
 *
 * @retval negative error code, number of bytes written on success.
 */
int efs_write(const char *abs_path, uint8_t *data, size_t size);

/** @brief Append to encrypted file
 *
 * @param abs_path directory path and name
 * @param data to be written
 * @param size in bytes
 *
 * @retval negative error code, number of bytes written on success.
 */
int efs_append(const char *abs_path, uint8_t *data, size_t size);

/** @brief Read entire contents of encrypted file
 *
 * @param abs_path directory path and name
 * @param data pointer to data
 * @param size maximum number of bytes to read
 *
 * @retval negative error code, number of bytes read on success.
 */
ssize_t efs_read(const char *abs_path, uint8_t *data, size_t size);

/** @brief Read block of encrypted file
 *
 * @param abs_path directory path and name
 * @param offset Byte offset into file to start reading
 * @param data pointer to data
 * @param size maximum number of bytes to read
 *
 * @retval negative error code, number of bytes read on success.
 */
ssize_t efs_read_block(const char *abs_path, int offset, uint8_t *data, size_t size);

/** @brief Get size of encrypted file
 *
 * @param abs_path directory path and name
 *
 * @retval negative error code, size of file on success
 */
ssize_t efs_get_file_size(const char *abs_path);

/** @brief Compute SHA256 of an encrypted file.
 *
 * Hash is zeroed on start.
 *
 * @param hash result
 * @param abs_path absolute file name
 * @param size of file in bytes
 *
 * @retval 0 on success, otherwise negative system error code.
 */
int efs_sha256(uint8_t hash[FSU_HASH_SIZE], const char *abs_path, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* __ENCRYPTED_FILE_STORAGE_H__ */
