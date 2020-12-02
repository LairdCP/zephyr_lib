/**
 * @file file_system_utilities.c
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(fsu);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <stdio.h>
#include <zephyr.h>
#include <device.h>
#include <fs/fs.h>

#ifdef CONFIG_FSU_HASH
#include <mbedtls/sha256.h>
#endif

#ifdef CONFIG_FILE_SYSTEM_LITTLEFS
#include <fs/littlefs.h>
#endif

#include "file_system_utilities.h"

/******************************************************************************/
/* Global Data Definitions                                                    */
/******************************************************************************/
#ifdef CONFIG_FILE_SYSTEM_LITTLEFS
K_MUTEX_DEFINE(lfs_init_mutex);
#endif

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
#ifdef CONFIG_FILE_SYSTEM_LITTLEFS
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);
static struct fs_mount_t littlefs_mnt = { .type = FS_LITTLEFS,
					  .fs_data = &cstorage,
					  .storage_dev = (void *)FLASH_AREA_ID(
						  lfs_storage),
					  .mnt_point = CONFIG_FSU_MOUNT_POINT };

static bool lfs_mounted;
#endif

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
int fsu_lfs_mount(void)
{
	int rc = -ENOSYS;
#ifdef CONFIG_FILE_SYSTEM_LITTLEFS
	k_mutex_lock(&lfs_init_mutex, K_FOREVER);
	if (!lfs_mounted) {
		rc = fs_mount(&littlefs_mnt);
		if (rc != 0) {
			LOG_ERR("Error mounting littlefs [%d]", rc);
		} else {
			lfs_mounted = true;
		}

		if (lfs_mounted) {
			struct fs_statvfs stats;
			int status = fs_statvfs(littlefs_mnt.mnt_point, &stats);
			if (status == 0) {
				LOG_DBG("Optimal transfer block size %lu",
					stats.f_bsize);
				LOG_DBG("Allocation unit size %lu",
					stats.f_frsize);
				LOG_DBG("Free blocks %lu", stats.f_bfree);
			}
		}
	} else {
		rc = 0;
	}
	k_mutex_unlock(&lfs_init_mutex);
#endif
	return rc;
}

void fsu_list_directory(const char *path)
{
	if (path == NULL) {
		LOG_ERR("Invalid path");
		return;
	}

	struct fs_dir_t dir = { 0 };
	int rc = fs_opendir(&dir, path);
	LOG_DBG("%s opendir: %d\n", log_strdup(path), rc);

	while (rc >= 0) {
		struct fs_dirent entry = { 0 };

		rc = fs_readdir(&dir, &entry);
		if (rc < 0) {
			break;
		}
		if (entry.name[0] == 0) {
			LOG_DBG("End of files\n");
			break;
		}
		LOG_DBG("  %c %u %s\n",
			(entry.type == FS_DIR_ENTRY_FILE) ? 'F' : 'D',
			entry.size, entry.name);
	}

	(void)fs_closedir(&dir);
}

struct fs_dirent *fsu_find(const char *path, const char *name, size_t *count,
			   enum fs_dir_entry_type entry_type)
{
	*count = 0;
	struct fs_dirent *results = NULL;
	struct fs_dir_t dir = { 0 };

	/* The name can be an empty string. */
	if (path == NULL || name == NULL) {
		LOG_ERR("Invalid path or name");
		return results;
	}

	/* Count matching items */
	int rc = fs_opendir(&dir, path);
	LOG_DBG("%s opendir: %d", log_strdup(path), rc);
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
		if ((entry.type == entry_type) &&
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
			if ((results[i].type == entry_type) &&
			    (strstr(results[i].name, name) != NULL)) {
				LOG_DBG(" %u %s", results[i].size,
					log_strdup(results[i].name));
				i++;
			}
		}
		(void)fs_closedir(&dir);
	}
	LOG_DBG("Found %d matching entries", *count);

	return results;
}

void fsu_free_found(struct fs_dirent *entry)
{
	if (entry != NULL) {
		k_free(entry);
	}
}

int fsu_sha256(uint8_t hash[FSU_HASH_SIZE], const char *path, const char *name,
	       size_t size)
{
	int rc = -1;

#ifdef CONFIG_FSU_HASH
	if (path == NULL || name == NULL) {
		LOG_ERR("Invalid path or name");
		return rc;
	}

	char abs_path[FSU_MAX_ABS_PATH_SIZE];
	(void)fsu_build_full_name(abs_path, sizeof(abs_path), path, name);
	memset(&hash[0], 0, FSU_HASH_SIZE);

	struct fs_file_t f;
	rc = fs_open(&f, abs_path);
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
#endif
	return rc;
}

int fsu_build_full_name(char *result, size_t max_size, const char *path,
			const char *name)
{
	if (path == NULL || name == NULL) {
		LOG_ERR("Invalid path or name");
		return -1;
	}

	if (result != NULL) {
		memset(result, 0, max_size);
	}
	return snprintk(result, max_size, "%s/%s", path, name);
}

int fsu_single_entry_exists(const char *path, const char *name,
			    enum fs_dir_entry_type entry_type)
{
	if (path == NULL || name == NULL) {
		LOG_ERR("Invalid path or name");
		return -1;
	}

	int status = -1;
	size_t count = 0;
	struct fs_dirent *pEntries = fsu_find(path, name, &count, entry_type);
	if (count == 0) {
		status = -ENOENT;
	} else if (count != 1) {
		status = -1;
	} else if (pEntries != NULL) { /* redundant check */
		status = pEntries->size;
	}
	fsu_free_found(pEntries);
	return status;
}

int fsu_append(const char *path, const char *name, void *data, size_t size)
{
	if (path == NULL || name == NULL) {
		LOG_ERR("Invalid path or name");
		return -1;
	}

	char abs_path[FSU_MAX_ABS_PATH_SIZE];
	(void)fsu_build_full_name(abs_path, sizeof(abs_path), path, name);

	return fsu_append_abs(abs_path, data, size);
}

int fsu_append_abs(const char *abs_path, void *data, size_t size)
{
	if (abs_path == NULL) {
		LOG_ERR("Invalid path + file name");
		return -1;
	}

	struct fs_file_t handle;
	int rc = fs_open(&handle, abs_path, FS_O_CREATE | FS_O_APPEND);
	if (rc < 0) {
		LOG_ERR("Unable to open file %s for append",
			log_strdup(abs_path));
		return rc;
	}

	rc = fs_write(&handle, data, size);
	if (rc < 0) {
		LOG_ERR("Unable to append file %s", log_strdup(abs_path));
	} else {
		LOG_DBG("%s apppend (%d)", log_strdup(abs_path), rc);
	}

	int rc2 = fs_close(&handle);
	if (rc2 < 0) {
		LOG_ERR("Unable to close file");
	}
	return rc;
}

int fsu_delete_files(const char *path, const char *name)
{
	if (path == NULL || name == NULL) {
		LOG_ERR("Invalid path or name");
		return -1;
	}

	char abs_path[FSU_MAX_ABS_PATH_SIZE];
	int status = 0;
	size_t count = 0;
	size_t i = 0;
	struct fs_dirent *pEntries =
		fsu_find(path, name, &count, FS_DIR_ENTRY_FILE);
	if (count == 0) {
		status = -ENOENT;
	} else {
		while (i < count) {
			(void)fsu_build_full_name(abs_path, sizeof(abs_path),
						  path, pEntries[i].name);
			LOG_DBG("Deleting (unlinking) file %s",
				log_strdup(abs_path));
			status = fs_unlink(abs_path);
			if (status == 0) {
				i += 1;
			} else {
				break;
			}
		}
	}
	fsu_free_found(pEntries);
	return i;
}

int fsu_mkdir(const char *path, const char *name)
{
	int r = fsu_single_entry_exists(path, name, FS_DIR_ENTRY_DIR);
	if (r < 0) {
		char abs_path[FSU_MAX_ABS_PATH_SIZE];
		(void)fsu_build_full_name(abs_path, sizeof(abs_path), path,
					  name);

		int r = fs_mkdir(abs_path);
		if (r < 0) {
			LOG_ERR("Unable to create directory %s",
				log_strdup(abs_path));
		}
	} else {
		LOG_DBG("Directory exists");
	}
	return r;
}
