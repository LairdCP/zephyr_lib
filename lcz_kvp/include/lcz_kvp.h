/**
 * @file lcz_kvp.h
 * @brief Methods for reading and writing key-value pairs (settings) to files in
 * human readable format.
 *
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_KVP_H__
#define __LCZ_KVP_H__

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************/
/* Global Constants, Macros and Type Definitions                                                  */
/**************************************************************************************************/
typedef struct lcz_kvp_cfg {
	size_t max_file_out_size;
	bool encrypted;
} lcz_kvp_cfg_t;

/* These may not be not null terminated (may point to locations in a buffer). */
typedef struct lcz_kvp {
	char *key;
	int key_len;
	char *val;
	int val_len;
} lcz_kvp_t;

/* Using quotes "" for empty string makes it easier to validate file because
 * an empty value can be treated as invalid.
 */
#define LCZ_KVP_EMPTY_VALUE_STR "\"\""

/**************************************************************************************************/
/* Global Function Prototypes                                                                     */
/**************************************************************************************************/
/**
 * @brief Write to the filesystem.
 *
 * @param encrypted true if file should be written encrypted, false otherwise
 * @param name file name
 * @param data pointer to data
 * @param size size of data
 *
 * @retval negative error code, 0 on success.
 */
ssize_t lcz_kvp_write(bool encrypted, char *name, void *data, size_t size);

/**
 * @brief Read from the filesystem.
 *
 * @param encrypted true if file is encrypted, false otherwise
 * @param name file name
 * @param data pointer to data
 * @param size max size of read
 *
 * @retval negative error code, bytes read on success.
 */
ssize_t lcz_kvp_read(bool encrypted, char *name, void *data, size_t size);

/**
 * @brief Delete file from the filesystem.
 *
 * @param name file name
 *
 * @retval negative error code, 0 on success
 */
int lcz_kvp_delete(char *name);

/**
 * @brief Parses a text file. Data is in hex with least significant byte first.
 *
 * @note Example File:
 *
 * @param cfg file configuration
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
int lcz_kvp_parse_from_file(const lcz_kvp_cfg_t *cfg, const char *fname, size_t *fsize, char **fstr,
			    lcz_kvp_t **kv);

/**
 * @brief Validate a file.
 *
 * @param cfg file configuration
 * @param str File as a string. Doesn't need to be be null terminated.
 * @param size length of file.
 *
 * @retval negative on error, otherwise number of key-value pairs.
 */
int lcz_kvp_validate_file(const lcz_kvp_cfg_t *cfg, const char *str, size_t size);

/**
 * @brief Generates a key-value pair file. Allocates buffer on first call and
 * appends to buffer on subsequent calls.
 *
 * @param cfg file configuration
 * @param kvp key-value pair
 * @param fstr pointer to string. Allocated by this function when pointing
 * to NULL.
 *
 * @note Caller is responsible for freeing fstr.
 *
 * @retval negative on error, otherwise number of bytes added to file string.
 */
int lcz_kvp_generate_file(const lcz_kvp_cfg_t *cfg, const lcz_kvp_t *kvp, char **fstr);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_KVP_H__ */
