/**
 * @file lcz_param_file.h
 * @brief Methods for reading and writing parameters (settings) to files in
 * binary format.  Reads and writes parameters to text files using key-value
 * pairs.
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_PARAM_FILE_H__
#define __LCZ_PARAM_FILE_H__

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
typedef enum param { PARAM_BIN = 0, PARAM_STR } param_t;

typedef uint16_t param_id_t;

typedef struct param_kvp {
	param_id_t id;
	char *keystr;
	int length;
} param_kvp_t;

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Write to the filesystem.
 *
 * @param name file name (prepended with CONFIG_LCZ_PARAM_FILE_MOUNT_POINT)
 * @param data pointer to data
 * @param size size of data
 *
 * @retval negative error code, 0 on success.
 */
ssize_t lcz_param_file_write(char *name, void *data, size_t size);

/**
 * @brief Read from the filesystem.
 *
 * @param name file name (prepended with CONFIG_LCZ_PARAM_FILE_MOUNT_POINT)
 * @param data pointer to data
 * @param size max size of read
 *
 * @retval negative error code, bytes read on success.
 */
ssize_t lcz_param_file_read(char *name, void *data, size_t size);

/**
 * @brief Delete parameter file from the filesystem.
 *
 * @param name file name (prepended with CONFIG_LCZ_PARAM_FILE_MOUNT_POINT)
 *
 * @retval negative error code, 0 on success
 */
int lcz_param_file_delete(char *name);

/**
 * @brief Override weak implementation in application to use a
 * different mount point than the one used by the FSU module.
 *
 * @retval negative error code, 0 on success
 */
int lcz_param_file_mount_fs(void);

/**
 * @brief Parses a parameter text file.  Data is in hex with least
 * significant byte first.
 *
 * @note Example File:
 * 0000=0A00\n
 * 0001=02\n
 * 0002=FA00\n
 * 0003=08CBDAFA01C308CBDAFA01C3\n
 * 0004=Laird Connectivity\n
 *
 * @param fname absolute path name of file
 * @param fsize pointer to size of file string (set by this function)
 * @param fstr pointer to file as a string (allocated by this function)
 * @param kv pointer to array of key-value pairs (allocated by this function)
 * The key-value pairs point to locations in the fstr
 *
 * @retval negative error code or number of key-value pairs found.
 *
 * @note If the return is non-negative, then it is the responsibility of the
 * caller to free fstr and kv.
 */
int lcz_param_file_parse_from_file(const char *fname, size_t *fsize,
				   char **fstr, param_kvp_t **kv);

/**
 * @brief Validate a parameter file.
 *
 * @param str parameter file as string. Doesn't need to be be null terminated.
 * @param size length of file.
 *
 * @retval negative on error, otherwise number of key-value pairs.
 */
int lcz_param_file_validate_file(const char *str, size_t size);

/**
 * @brief Generates a parameter file.  Allocates buffer on first call and
 * appends to buffer on subsequent calls.
 *
 * @param id The id of the parameter to add.
 * @param type The type of the parameter.
 * @param data pointer
 * @param dsize size of the data in bytes
 * @param fstr pointer to string.  Allocated by this function when pointing
 * to NULL.
 *
 * @note Caller is responsible for freeing fstr.
 *
 * @retval negative on error, otherwise number of bytes added to file string.
 */
int lcz_param_file_generate_file(param_id_t id, param_t type, const void *data,
				 size_t dsize, char **fstr);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_PARAM_FILE_H__ */
