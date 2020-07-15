/**
 * @file file_system_utilities.c
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(fsu);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <stdio.h>
#include <zephyr.h>
#include <device.h>
#include <fs/fs.h>
#include <mbedtls/sha256.h>

#include "file_system_utilities.h"

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void fsu_list_directory(const char *path)
{
	struct fs_dir_t dir = { 0 };
	int rc = fs_opendir(&dir, path);
	printk("%s opendir: %d\n", path, rc);

	while (rc >= 0) {
		struct fs_dirent entry = { 0 };

		rc = fs_readdir(&dir, &entry);
		if (rc < 0) {
			break;
		}
		if (entry.name[0] == 0) {
			printk("End of files\n");
			break;
		}
		printk("  %c %u %s\n",
		       (entry.type == FS_DIR_ENTRY_FILE) ? 'F' : 'D',
		       entry.size, entry.name);
	}

	(void)fs_closedir(&dir);
}

struct fs_dirent *fsu_find(const char *path, const char *name, size_t *count)
{
	*count = 0;
	struct fs_dirent *results = NULL;
	struct fs_dir_t dir = { 0 };

	/* Count matching items */
	int rc = fs_opendir(&dir, path);
	LOG_DBG("%s opendir: %d", path, rc);
	struct fs_dirent entry;
	while (rc >= 0) {
		memset(&entry, 0, sizeof(struct fs_dirent));
		rc = fs_readdir(&dir, &entry);
		if (rc < 0) {
			break;
		}
		if (entry.name[0] == 0) {
			LOG_DBG("End of files");
			break;
		}
		if ((entry.type == FS_DIR_ENTRY_FILE) &&
		    (strstr(entry.name, name) != NULL)) {
			(*count)++;
		}
	}
	(void)fs_closedir(&dir);

	/* Make an array of matching items. */
	if (*count > 0) {
		results = k_calloc(*count, sizeof(struct fs_dirent));
		rc = fs_opendir(&dir, path);
		size_t i = 0;
		while ((i < *count) && (rc >= 0) && (results != NULL)) {
			rc = fs_readdir(&dir, &results[i]);
			if (rc < 0) {
				LOG_ERR("Unexpected file find error");
				*count = i;
				break;
			}
			if ((results[i].type == FS_DIR_ENTRY_FILE) &&
			    (strstr(results[i].name, name) != NULL)) {
				LOG_DBG(" %u %s", results[i].size,
					log_strdup(results[i].name));
				i++;
			}
		}
		(void)fs_closedir(&dir);
	}
	LOG_DBG("Found %d matching files", *count);

	return results;
}

void fsu_free_found(struct fs_dirent *entry)
{
	if (entry != NULL) {
		k_free(entry);
	}
}

int fsu_sha256(const char *abs_path, size_t size, uint8_t hash[FSU_HASH_SIZE])
{
	memset(&hash[0], 0, FSU_HASH_SIZE);

	struct fs_file_t f;
	int rc = fs_open(&f, abs_path);
	if (rc < 0) {
		return rc;
	}

	uint8_t *pBuffer = k_malloc(CONFIG_FSU_HASH_CHUNK_SIZE);
	mbedtls_sha256_context *pCtx = k_malloc(sizeof(mbedtls_sha256_context));
	if ((pBuffer != NULL) && (pCtx != NULL)) {
		mbedtls_sha256_init(pCtx);
		rc = mbedtls_sha256_starts_ret(pCtx, 0);

		size_t rem = size;
		ssize_t bytes_read;
		size_t length;
		while (rc == 0 && rem > 0) {
			length = MIN(rem, CONFIG_FSU_HASH_CHUNK_SIZE);
			bytes_read = fs_read(&f, pBuffer, length);
			if (bytes_read == length) {
				rc = mbedtls_sha256_update_ret(pCtx, pBuffer,
							       length);
				rem -= length;
			} else {
				rc = -EIO;
			}
		}

		if (rc == 0 && rem == 0) {
			rc = mbedtls_sha256_finish_ret(pCtx, hash);
		}
	} else {
		rc = -ENOMEM;
	}

	if (pBuffer != NULL) {
		k_free(pBuffer);
	}
	if (pCtx != NULL) {
		k_free(pCtx);
	}
	(void)fs_close(&f);
	return rc;
}