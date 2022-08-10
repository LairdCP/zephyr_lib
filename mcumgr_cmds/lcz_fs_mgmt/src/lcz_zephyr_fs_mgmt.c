/**
 * @file lcz_zephyr_fs_mgmt.c
 *
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: LicenseRef-LairdConnectivity-Clause
 */

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <fs/fs.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <mgmt/mcumgr/buf.h>
#include <mgmt/mgmt.h>
#include "file_system_utilities.h"
#if defined(CONFIG_FSU_ENCRYPTED_FILES)
#include "encrypted_file_storage.h"
#endif
#include <lcz_fs_mgmt/lcz_fs_mgmt_impl.h>

/**************************************************************************************************/
/* Global Function Definitions                                                                    */
/**************************************************************************************************/
int lcz_fs_mgmt_impl_filelen(const char *path, size_t *out_len)
{
	ssize_t size;

#if defined(CONFIG_FSU_ENCRYPTED_FILES)
	if (efs_is_encrypted_path(path)) {
		size = efs_get_file_size(path);
	} else
#endif
	{
		size = fsu_get_file_size_abs(path);
	}

	if (size >= 0) {
		if (out_len != NULL) {
			*out_len = size;
		}
		return 0;
	} else {
		return size;
	}
}

int lcz_fs_mgmt_impl_read(const char *path, size_t offset, size_t len, void *out_data,
			  size_t *out_len)
{
	struct fs_file_t file;
	ssize_t bytes_read;
	int rc;

#if defined(CONFIG_FSU_ENCRYPTED_FILES)
	if (efs_is_encrypted_path(path)) {
		bytes_read = efs_read_block(path, offset, out_data, len);
		if (bytes_read >= 0) {
			if (out_len != NULL) {
				*out_len = bytes_read;
			}
			rc = 0;
		} else {
			rc = bytes_read;
		}
	} else
#endif
	{
		fs_file_t_init(&file);
		rc = fs_open(&file, path, FS_O_READ);
		if (rc != 0) {
			return MGMT_ERR_ENOENT;
		}

		rc = fs_seek(&file, offset, FS_SEEK_SET);
		if (rc != 0) {
			goto done;
		}

		bytes_read = fs_read(&file, out_data, len);
		if (bytes_read < 0) {
			rc = bytes_read;
			goto done;
		}

		*out_len = bytes_read;

	done:
		fs_close(&file);
	}

	if (rc < 0) {
		return MGMT_ERR_EUNKNOWN;
	}
	return 0;
}

int lcz_fs_mgmt_impl_write(const char *path, size_t offset, const void *data, size_t len)
{
	struct fs_file_t file;
	ssize_t file_size;
	int rc;

#if defined(CONFIG_FSU_ENCRYPTED_FILES)
	if (efs_is_encrypted_path(path)) {
		if (offset == 0) {
			rc = efs_write(path, (uint8_t *)data, len);
		} else {
			file_size = efs_get_file_size(path);
			if (offset == file_size) {
				rc = efs_append(path, (uint8_t *)data, len);
			} else {
				/* Can't write to middle of the file, only append to the end */
				rc = MGMT_ERR_EINVAL;
			}
		}
	} else
#endif
	{
		/* Truncate the file before writing the first chunk.  This is done to
		 * properly handle an overwrite of an existing file
		 */
		if (offset == 0) {
			/* Try to truncate file; this will return -ENOENT if file
			 * does not exist so ignore it. Fs may log error -2 here,
			 * just ignore it.
			 */
			rc = fs_unlink(path);
			if (rc < 0 && rc != -ENOENT) {
				return rc;
			}
		}

		fs_file_t_init(&file);
		rc = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE);
		if (rc != 0) {
			return MGMT_ERR_EUNKNOWN;
		}

		rc = fs_seek(&file, offset, FS_SEEK_SET);
		if (rc != 0) {
			goto done;
		}

		rc = fs_write(&file, data, len);
		if (rc < 0) {
			goto done;
		}

	done:
		fs_close(&file);
	}

	if (rc < 0) {
		return MGMT_ERR_EUNKNOWN;
	}
	return 0;
}

int lcz_fs_mgmt_impl_hash(const char *path, size_t size, uint8_t output[FSU_HASH_SIZE])
{
#if defined(CONFIG_FSU_ENCRYPTED_FILES)
	if (efs_is_encrypted_path(path)) {
		return efs_sha256(output, path, size);
	} else
#endif
	{
		return fsu_sha256_abs(output, path, size);
	}
}
