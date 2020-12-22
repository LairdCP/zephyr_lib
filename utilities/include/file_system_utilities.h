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

#define FSU_MAX_PATH_STR_LEN (CONFIG_FSU_MAX_PATH_SIZE - 1)
#define FSU_MAX_FILE_NAME_LEN (CONFIG_FSU_MAX_FILE_NAME_SIZE - 1)

#define FSU_MAX_ABS_PATH_SIZE                                                  \
	(CONFIG_FSU_MAX_PATH_SIZE + CONFIG_FSU_MAX_FILE_NAME_SIZE - 1)

#define FSU_MAX_ABS_PATH_LEN (FSU_MAX_ABS_PATH_SIZE - 1)

/* app_1.2.3.4.bin, something.txt */
BUILD_ASSERT(CONFIG_FSU_MAX_FILE_NAME_SIZE >=
		     (CONFIG_FSU_MAX_VERSION_SIZE + 1 +
		      CONFIG_FSU_MAX_IMAGE_NAME_SIZE + 4),
	     "File name too small");

/* CONFIG_FILE_SYSTEM_MAX_FILE_NAME can be used to reduce memory requirements
 * for directory operations.
 */
BUILD_ASSERT(CONFIG_FSU_MAX_FILE_NAME_SIZE < MAX_FILE_NAME,
	     "File name too large");

/* An empty string will match everything */
#define FSU_EMPTY_STRING ""

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Mount LittleFS to /lfs
 *
 * @retval 0 on success, otherwise negative system error code.
 */
int fsu_lfs_mount(void);

/**
 * @brief print all files in a directory to the debug log.
 */
void fsu_list_directory(const char *path);

/**
 * @brief Find files that match name.
 *
 * @param path directory path
 * @param name file or folder name
 * @param count number of entries with a matching name
 * @param entry_type file or folder
 * @note This doesn't traverse directories.
 *
 * @retval list of entries (malloced)
 */
struct fs_dirent *fsu_find(const char *path, const char *name, size_t *count,
			   enum fs_dir_entry_type entry_type);

/**
 * @brief Free memory allocated by fsu_find.
 *
 * @param entry pointer to list of entries
 */
void fsu_free_found(struct fs_dirent *entry);

/**
 * @brief Compute SHA256 of a file.  Hash is zeroed on start.
 *
 * @param hash result
 * @param path mount point or folder path
 * @param name filename
 * @param size of file in bytes
 *
 * @retval 0 on success, otherwise negative system error code.
 */
int fsu_sha256(uint8_t hash[FSU_HASH_SIZE], const char *path, const char *name,
	       size_t size);

/**
 * @brief Build name with path "%s/%s" using snprintk.
 * Result is zeroed on entry.
 *
 * @param result output
 * @param max_size number of bytes in string
 * @param path mount point or folder path
 * @param name filename
 *
 * @retval The number of characters that would have been written
 * not counting null char.
 * If an encoding error occurs, a negative number is returned.
 */
int fsu_build_full_name(char *result, size_t max_size, const char *path,
			const char *name);

/**
 * @brief Searches directory for a single matching file.
 *
 * @param path directory path
 * @param name file or folder name
 * @param entry_type file or folder
 *
 * @note It is more efficient to use get_file_size to determine
 * if a file exists.
 *
 * @retval negative error code; if found, size of file.
 */
int fsu_single_entry_exists(const char *path, const char *name,
			    enum fs_dir_entry_type entry_type);

/**
 * @brief Opens file, appends data, and closes file.
 *
 * @param path directory path
 * @param name file name
 * @param data to be written
 * @param size in bytes
 *
 * @retval negative error code, number of bytes written on success.
 */
int fsu_append(const char *path, const char *name, void *data, size_t size);

/**
 * @brief Opens file, writes data, and closes file.
 *
 * @param path directory path
 * @param name file name
 * @param data to be written
 * @param size in bytes
 *
 * @retval negative error code, number of bytes written on success.
 */
int fsu_write(const char *path, const char *name, void *data, size_t size);

/**
 * @brief Opens file, appends data, and closes file.
 *
 * @param abs_path directory path and name
 * @param data to be written
 * @param size in bytes
 *
 * @retval negative error code, number of bytes written on success.
 */
int fsu_append_abs(const char *abs_path, void *data, size_t size);

/**
 * @brief Opens file, writes data, and closes file.
 *
 * @param abs_path directory path and name
 * @param data to be written
 * @param size in bytes
 *
 * @retval negative error code, number of bytes written on success.
 */
int fsu_write_abs(const char *abs_path, void *data, size_t size);

/**
 * @brief Deletes all files matching name.
 *
 * @param path directory path
 * @param name file name (or partial name)
 *
 * @retval negative error code, number of deleted files on success
 */
int fsu_delete_files(const char *path, const char *name);

/**
 * @brief Creates a directory if it doesn't exist.
 *
 * @param path directory path
 * @param name folder name
 *
 * @retval negative error code, 0 on success
 */
int fsu_mkdir(const char *path, const char *name);

/**
 * @brief Opens file, writes data, and closes file.
 *
 * @param path directory path
 * @param name file name
 * @param data pointer to data
 * @param size maximum number of bytes to read
 *
 * @retval negative error code, number of bytes read on success.
 */
ssize_t fsu_read(const char *path, const char *name, void *data, size_t size);

/**
 * @brief Opens file, writes data, and closes file.
 *
 * @param abs_path directory path and name
 * @param data pointer to data
 * @param size maximum number of bytes to read
 *
 * @retval negative error code, number of bytes read on success.
 */
ssize_t fsu_read_abs(const char *abs_path, void *data, size_t size);

/**
 * @brief Get size of file
 *
 * @param abs_path directory path and name
 *
 * @retval negative error code, size of file on success
 */
ssize_t fsu_get_file_size_abs(const char *abs_path);

/**
 * @brief Get size of file
 *
 * @param path directory path
 * @param name file name
 *
 * @retval negative error code, size of file on success
 */
ssize_t fsu_get_file_size(const char *path, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* __FILE_SYSTEM_UTILITIES_H__ */
