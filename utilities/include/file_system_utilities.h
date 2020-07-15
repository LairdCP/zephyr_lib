/**
 * @file file_system_utilities.h
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __FILE_SYSTEM_UTILITIES_H__
#define __FILE_SYSTEM_UTILITIES_H__

/* (Remove Empty Sections) */
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
/* Global Constants, Macros and Type Definitions                              */
/******************************************************************************/
#define FSU_HASH_SIZE 32

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief print all files in a directory
 */
void fsu_list_directory(const char *path);

/**
 * @brief Find files that match name.
 *
 * @param path directory path
 * @param name file name
 * @param count number of files with a matching name
 * @note This doesn't traverse directories.
 *
 * @retval list of entries (malloced)
 */
struct fs_dirent *fsu_find(const char *path, const char *name, size_t *count);

/**
 * @brief Free memory allocated by fsu_find.
 *
 * @param entry pointer to list of entries
 */
void fsu_free_found(struct fs_dirent *entry);

/**
 * @brief Compute SHA256 of a file.
 *
 * @param abs_path of file
 * @param size of file in bytes
 * @param hash result
 *
 * @retval 0 on success, otherwise negative system error code.
 */
int fsu_sha256(const char *abs_path, size_t size, uint8_t hash[FSU_HASH_SIZE]);

#ifdef __cplusplus
}
#endif

#endif /* __FILE_SYSTEM_UTILITIES_H__ */
