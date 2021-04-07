/**
 * @file file_system_utilities.c
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(fsu, CONFIG_FSU_LOG_LEVEL);

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
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define BREAK_ON_ERROR(x)                                                      \
	if (x < 0) {                                                           \
		break;                                                         \
	}

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
/* Local Function Prototypes                                                  */
/******************************************************************************/
static ssize_t fsu_wa(const char *path, const char *name, void *data,
		      size_t size, bool append);

static ssize_t fsu_wa_abs(const char *abs_path, void *data, size_t size,
			  bool append);

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
				LOG_INF("Optimal transfer block size %lu",
					stats.f_bsize);
				LOG_INF("Allocation unit size %lu",
					stats.f_frsize);
				LOG_INF("Free blocks %lu", stats.f_bfree);
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
	/* Use malloc because entry is 264 bytes when using LFS */
	struct fs_dirent *entry = k_malloc(sizeof(struct fs_dirent));
	if (entry == NULL) {
		LOG_ERR("Unable to allocate directory entry");
	}
	while (rc >= 0 && (entry != NULL)) {
		memset(entry, 0, sizeof(struct fs_dirent));
		rc = fs_readdir(&dir, entry);
		if (rc < 0) {
			break;
		}
		if (entry->name[0] == 0) {
			LOG_DBG("End of files");
			break;
		}
		if ((entry->type == entry_type) &&
		    (strstr(entry->name, name) != NULL)) {
			(*count)++;
		}
	}
	(void)k_free(entry);
	(void)fs_closedir(&dir);

	/* Make an array of matching items. */
	if (*count > 0) {
		results = k_calloc(*count, sizeof(struct fs_dirent));
		if (results == NULL) {
			LOG_ERR("Unable to allocate directory entries");
		}
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
	int rc = -EPERM;
	char abs_path[FSU_MAX_ABS_PATH_SIZE];

	if (path == NULL || name == NULL) {
		LOG_ERR("Invalid path or name");
		return rc;
	}

	(void)fsu_build_full_name(abs_path, sizeof(abs_path), path, name);

	return fsu_sha256_abs(hash, abs_path, size);
}

int fsu_sha256_abs(uint8_t hash[FSU_HASH_SIZE], const char *abs_path,
		   size_t size)
{
	int rc = -EPERM;
	struct fs_file_t f;

	memset(&hash[0], 0, FSU_HASH_SIZE);

#ifdef CONFIG_FSU_HASH
	rc = fs_open(&f, abs_path, FS_O_READ);
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
		return -EPERM;
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
		return -EPERM;
	}

	int status = -EPERM;
	size_t count = 0;
	struct fs_dirent *pEntries = fsu_find(path, name, &count, entry_type);
	if (count == 0) {
		status = -ENOENT;
	} else if (count != 1) {
		status = -EINVAL;
	} else if (pEntries != NULL) { /* redundant check */
		status = pEntries->size;
	}
	fsu_free_found(pEntries);
	return status;
}

ssize_t fsu_append(const char *path, const char *name, void *data, size_t size)
{
	return fsu_wa(path, name, data, size, true);
}

ssize_t fsu_write(const char *path, const char *name, void *data, size_t size)
{
	return fsu_wa(path, name, data, size, false);
}

ssize_t fsu_append_abs(const char *abs_path, void *data, size_t size)
{
	return fsu_wa_abs(abs_path, data, size, true);
}

ssize_t fsu_write_abs(const char *abs_path, void *data, size_t size)
{
	return fsu_wa_abs(abs_path, data, size, false);
}

int fsu_delete(const char *path, const char *name)
{
	char abs_path[FSU_MAX_ABS_PATH_SIZE];

	if (path == NULL || name == NULL) {
		LOG_ERR("Invalid path or name");
		return -EPERM;
	}

	(void)fsu_build_full_name(abs_path, sizeof(abs_path), path, name);
	return fsu_delete_abs(abs_path);
}

int fsu_delete_abs(const char *abs_path)
{
	LOG_DBG("Deleting (unlinking) file %s", log_strdup(abs_path));
	return fs_unlink(abs_path);
}

int fsu_delete_files(const char *path, const char *name)
{
	if (path == NULL || name == NULL) {
		LOG_ERR("Invalid path or name");
		return -EPERM;
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
	char abs_path[FSU_MAX_ABS_PATH_SIZE];
	int r = fsu_single_entry_exists(path, name, FS_DIR_ENTRY_DIR);
	if (r < 0) {
		r = fsu_build_full_name(abs_path, sizeof(abs_path), path, name);
		if (r >= 0) {
			r = fs_mkdir(abs_path);
			if (r < 0) {
				LOG_ERR("Unable to create directory %s",
					log_strdup(abs_path));
			}
		}
	} else {
		LOG_DBG("Directory exists");
	}
	return r;
}

ssize_t fsu_read(const char *path, const char *name, void *data, size_t size)
{
	ssize_t r = -EPERM;
	char abs_path[FSU_MAX_ABS_PATH_SIZE];

	do {
		if (path == NULL || name == NULL) {
			LOG_ERR("Invalid path or name");
			break;
		}

		r = fsu_build_full_name(abs_path, sizeof(abs_path), path, name);
		BREAK_ON_ERROR(r);

		r = fsu_read_abs(abs_path, data, size);

	} while (0);

	return r;
}

ssize_t fsu_read_abs(const char *abs_path, void *data, size_t size)
{
	ssize_t r = -EPERM;

	do {
		if (abs_path == NULL) {
			LOG_ERR("Invalid absolute path");
			break;
		}

		/* It is quicker to try to open the file than checking if it exists. */
		struct fs_file_t f;
		r = fs_open(&f, abs_path, FS_O_READ);
		BREAK_ON_ERROR(r);

		r = fs_read(&f, data, size);

		int rc2 = fs_close(&f);
		if (rc2 < 0) {
			LOG_ERR("Unable to close file");
			/* Don't mask other errors */
			if (r >= 0) {
				r = rc2;
			}
		}

	} while (0);

	return r;
}

ssize_t fsu_get_file_size_abs(const char *abs_path)
{
	ssize_t r = -EPERM;
	struct fs_dirent *entry = k_malloc(sizeof(struct fs_dirent));

	do {
		if (entry == NULL) {
			r = -ENOMEM;
			LOG_ERR("Allocation failure");
			break;
		}

		r = fs_stat(abs_path, entry);
		if (r < 0) {
			LOG_WRN("%s not found", log_strdup(abs_path));
			break;
		} else {
			r = entry->size;
		}

	} while (0);

	k_free(entry);

	return r;
}

ssize_t fsu_get_file_size(const char *path, const char *name)
{
	ssize_t r = -EPERM;
	char abs_path[FSU_MAX_ABS_PATH_SIZE];

	do {
		if (path == NULL || name == NULL) {
			LOG_ERR("Invalid path or name");
			break;
		}

		r = fsu_build_full_name(abs_path, sizeof(abs_path), path, name);
		BREAK_ON_ERROR(r);

		r = fsu_get_file_size_abs(abs_path);

	} while (0);

	return r;
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
/* write or append */
static ssize_t fsu_wa(const char *path, const char *name, void *data,
		      size_t size, bool append)
{
	if (path == NULL || name == NULL) {
		LOG_ERR("Invalid path or name");
		return -EPERM;
	}

	char abs_path[FSU_MAX_ABS_PATH_SIZE];
	(void)fsu_build_full_name(abs_path, sizeof(abs_path), path, name);

	return fsu_wa_abs(abs_path, data, size, append);
}

static ssize_t fsu_wa_abs(const char *abs_path, void *data, size_t size,
			  bool append)
{
	fs_mode_t flags = FS_O_CREATE | (append ? FS_O_APPEND : FS_O_WRITE);
	const char *desc = append ? "append" : "write";
	ssize_t rc = -EPERM;

	do {
		if (abs_path == NULL) {
			LOG_ERR("Invalid path + file name");
			break;
		}

		struct fs_file_t handle;
		rc = fs_open(&handle, abs_path, flags);
		if (rc < 0) {
			LOG_ERR("Unable to open file %s for %s",
				log_strdup(abs_path), desc);
		}
		BREAK_ON_ERROR(rc);

		/* When rewriting a file the size must be updated. */
		if (rc >= 0 && !append) {
			rc = fs_truncate(&handle, size);
		}

		rc = fs_write(&handle, data, size);
		if (rc < 0) {
			LOG_ERR("Unable to %s file %s", desc,
				log_strdup(abs_path));
		} else if (rc != size) {
			rc = -ENOSPC;
			LOG_ERR("Disk Full: Unable to %s file %s", desc,
				log_strdup(abs_path));
		} else {
			LOG_DBG("%s %s (%d)", log_strdup(abs_path), desc, rc);
		}

		int rc2 = fs_close(&handle);
		if (rc2 < 0) {
			LOG_ERR("Unable to close file");
			/* Don't mask other errors */
			if (rc >= 0) {
				rc = rc2;
			}
		}

	} while (0);

#ifdef CONFIG_FSU_REWRITE_SIZE_CHECK
	if (rc >= 0 && !append) {
		ssize_t read_size = fsu_get_file_size_abs(abs_path);
		if (read_size != size) {
			LOG_ERR("Unexpected file size (actual) %d != %d (desired)",
				read_size, size);
			rc = -EIO;
		}
	}
#endif

	return rc;
}
