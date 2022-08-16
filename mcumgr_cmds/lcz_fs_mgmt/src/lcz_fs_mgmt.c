/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#define LOG_LEVEL CONFIG_MCUMGR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(fs_mgmt);

#include <assert.h>
#include <zephyr.h>
#include <init.h>
#include <limits.h>
#include <string.h>
#include <sys/util.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include "zcbor_bulk/zcbor_bulk_priv.h"
#include <mgmt/mcumgr/buf.h>
#include <fs/fs.h>
#include "mgmt/mgmt.h"
#include "file_system_utilities.h"
#include "lcz_fs_mgmt/lcz_fs_mgmt.h"
#include "lcz_fs_mgmt/lcz_fs_mgmt_impl.h"
#include "lcz_fs_mgmt/lcz_fs_mgmt_config.h"

/**************************************************************************************************/
/* Local Constant, Macro and Type Definitions                                                     */
/**************************************************************************************************/
#define HASH_CHECKSUM_TYPE_SIZE 8

static struct {
	/** Whether an upload is currently in progress. */
	bool uploading;

	/** Expected offset of next upload request. */
	size_t off;

	/** Total length of file currently being uploaded. */
	size_t len;
} fs_mgmt_ctxt;

/**************************************************************************************************/
/* Local Function Prototypes                                                                      */
/**************************************************************************************************/
static bool fs_mgmt_file_rsp(zcbor_state_t *zse, int rc, uint64_t off);
static int fs_mgmt_file_download(struct mgmt_ctxt *ctxt);
static int fs_mgmt_file_upload(struct mgmt_ctxt *ctxt);
#if defined(CONFIG_LCZ_FS_MGMT_FILE_STATUS)
static int fs_mgmt_file_status(struct mgmt_ctxt *ctxt);
#endif
#if defined(CONFIG_LCZ_FS_MGMT_CHECKSUM_HASH)
static int fs_mgmt_file_hash_checksum(struct mgmt_ctxt *ctxt);
#endif

/**************************************************************************************************/
/* Local Data Definitions                                                                         */
/**************************************************************************************************/
static const struct mgmt_handler fs_mgmt_handlers[] = {
	[FS_MGMT_ID_FILE] = {
		.mh_read = fs_mgmt_file_download,
		.mh_write = fs_mgmt_file_upload,
	},
#if defined(CONFIG_LCZ_FS_MGMT_FILE_STATUS)
	[FS_MGMT_ID_STAT] = {
		.mh_read = fs_mgmt_file_status,
		.mh_write = NULL,
	},
#endif
#if defined(CONFIG_LCZ_FS_MGMT_CHECKSUM_HASH)
	[FS_MGMT_ID_HASH_CHECKSUM] = {
		.mh_read = fs_mgmt_file_hash_checksum,
		.mh_write = NULL,
	},
#endif
};

#define FS_MGMT_HANDLER_CNT ARRAY_SIZE(fs_mgmt_handlers)

static struct mgmt_group fs_mgmt_group = {
	.mg_handlers = fs_mgmt_handlers,
	.mg_handlers_count = FS_MGMT_HANDLER_CNT,
	.mg_group_id = MGMT_GROUP_ID_FS,
};

#ifdef CONFIG_LCZ_FS_MGMT_FILE_ACCESS_HOOK
static fs_mgmt_on_evt_cb fs_evt_cb;
#endif

/**************************************************************************************************/
/* Local Function Definitions                                                                     */
/**************************************************************************************************/
/**
 * Encodes a file upload response.
 */
static bool fs_mgmt_file_rsp(zcbor_state_t *zse, int rc, uint64_t off)
{
	bool ok;

	ok = zcbor_tstr_put_lit(zse, "rc") && zcbor_int32_put(zse, rc) &&
	     zcbor_tstr_put_lit(zse, "off") && zcbor_uint64_put(zse, off);

	return ok;
}

/**
 * Command handler: fs file (read)
 */
static int fs_mgmt_file_download(struct mgmt_ctxt *ctxt)
{
	uint8_t file_data[FS_MGMT_DL_CHUNK_SIZE];
	char path[CONFIG_LCZ_FS_MGMT_PATH_SIZE + 1];
	uint64_t off = ULLONG_MAX;
	size_t bytes_read;
	size_t file_len;
	int rc;
	zcbor_state_t *zse = ctxt->cnbe->zs;
	zcbor_state_t *zsd = ctxt->cnbd->zs;
	bool ok;
	struct zcbor_string name = { 0 };
	size_t decoded;

	struct zcbor_map_decode_key_val fs_download_decode[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(off, zcbor_uint64_decode, &off),
		ZCBOR_MAP_DECODE_KEY_VAL(name, zcbor_tstr_decode, &name),
	};

	ok = zcbor_map_decode_bulk(zsd, fs_download_decode, ARRAY_SIZE(fs_download_decode),
				   &decoded) == 0;

	if (!ok || name.len == 0 || name.len > (sizeof(path) - 1)) {
		return MGMT_ERR_EINVAL;
	}

	memcpy(path, name.value, name.len);
	path[name.len] = '\0';

#ifdef CONFIG_LCZ_FS_MGMT_FILE_ACCESS_HOOK
	if (fs_evt_cb != NULL) {
		/* Send request to application to check if access should be allowed or not */
		if (fs_evt_cb(path, false) == false) {
			return MGMT_ERR_ENOTSUP;
		}
	}
#endif

	/* Only the response to the first download request contains the total file
	 * length.
	 */
	if (off == 0) {
		rc = lcz_fs_mgmt_impl_filelen(path, &file_len);
		if (rc != 0) {
			return rc;
		}
	}

	/* Read the requested chunk from the file. */
	rc = lcz_fs_mgmt_impl_read(path, off, FS_MGMT_DL_CHUNK_SIZE, file_data, &bytes_read);
	if (rc != 0) {
		return rc;
	}

	/* Encode the response. */
	ok = fs_mgmt_file_rsp(zse, MGMT_ERR_EOK, off) && zcbor_tstr_put_lit(zse, "data") &&
	     zcbor_bstr_encode_ptr(zse, file_data, bytes_read) &&
	     ((off != 0) || (zcbor_tstr_put_lit(zse, "len") && zcbor_uint64_put(zse, file_len)));

	return ok ? MGMT_ERR_EOK : MGMT_ERR_ENOMEM;
}

/**
 * Command handler: fs file (write)
 */
static int fs_mgmt_file_upload(struct mgmt_ctxt *ctxt)
{
	char file_name[CONFIG_LCZ_FS_MGMT_PATH_SIZE + 1];
	unsigned long long len = ULLONG_MAX;
	unsigned long long off = ULLONG_MAX;
	size_t new_off;
	bool ok;
	int rc;
	zcbor_state_t *zse = ctxt->cnbe->zs;
	zcbor_state_t *zsd = ctxt->cnbd->zs;
	struct zcbor_string name = { 0 };
	struct zcbor_string file_data = { 0 };
	size_t decoded = 0;

	struct zcbor_map_decode_key_val fs_upload_decode[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(off, zcbor_uint64_decode, &off),
		ZCBOR_MAP_DECODE_KEY_VAL(name, zcbor_tstr_decode, &name),
		ZCBOR_MAP_DECODE_KEY_VAL(data, zcbor_bstr_decode, &file_data),
		ZCBOR_MAP_DECODE_KEY_VAL(len, zcbor_uint64_decode, &len),
	};

	ok = zcbor_map_decode_bulk(zsd, fs_upload_decode, ARRAY_SIZE(fs_upload_decode), &decoded) ==
	     0;

	if (!ok || off == ULLONG_MAX || name.len == 0 || name.len > (sizeof(file_name) - 1)) {
		return MGMT_ERR_EINVAL;
	}

	memcpy(file_name, name.value, name.len);
	file_name[name.len] = '\0';

#ifdef CONFIG_LCZ_FS_MGMT_FILE_ACCESS_HOOK
	if (fs_evt_cb != NULL) {
		/* Send request to application to check if access should be allowed or not */
		if (fs_evt_cb(file_name, true) == false) {
			return MGMT_ERR_ENOTSUP;
		}
	}
#endif

	if (off == 0) {
		/* Total file length is a required field in the first chunk request. */
		if (len == ULLONG_MAX) {
			return MGMT_ERR_EINVAL;
		}

		fs_mgmt_ctxt.uploading = true;
		fs_mgmt_ctxt.off = 0;
		fs_mgmt_ctxt.len = len;
	} else {
		if (!fs_mgmt_ctxt.uploading) {
			return MGMT_ERR_EINVAL;
		}

		if (off != fs_mgmt_ctxt.off) {
			/* Invalid offset.  Drop the data and send the expected offset. */
			return fs_mgmt_file_rsp(zse, MGMT_ERR_EINVAL, fs_mgmt_ctxt.off);
		}
	}

	new_off = fs_mgmt_ctxt.off + file_data.len;
	if (new_off > fs_mgmt_ctxt.len) {
		/* Data exceeds image length. */
		return MGMT_ERR_EINVAL;
	}

	if (file_data.len > 0) {
		/* Write the data chunk to the file. */
		rc = lcz_fs_mgmt_impl_write(file_name, off, file_data.value, file_data.len);
		if (rc != 0) {
			return rc;
		}
		fs_mgmt_ctxt.off = new_off;
	}

	if (fs_mgmt_ctxt.off == fs_mgmt_ctxt.len) {
		/* Upload complete. */
		fs_mgmt_ctxt.uploading = false;
	}

	/* Send the response. */
	return fs_mgmt_file_rsp(zse, MGMT_ERR_EOK, fs_mgmt_ctxt.off) ? MGMT_ERR_EOK :
									     MGMT_ERR_ENOMEM;
}

#if defined(CONFIG_LCZ_FS_MGMT_FILE_STATUS)
/**
 * Command handler: fs stat (read)
 */
static int fs_mgmt_file_status(struct mgmt_ctxt *ctxt)
{
	char path[CONFIG_LCZ_FS_MGMT_PATH_SIZE + 1];
	size_t file_len;
	int rc;
	zcbor_state_t *zse = ctxt->cnbe->zs;
	zcbor_state_t *zsd = ctxt->cnbd->zs;
	bool ok;
	struct zcbor_string name = { 0 };
	size_t decoded;

	struct zcbor_map_decode_key_val fs_status_decode[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(name, zcbor_tstr_decode, &name),
	};

	ok = zcbor_map_decode_bulk(zsd, fs_status_decode, ARRAY_SIZE(fs_status_decode), &decoded) ==
	     0;

	if (!ok || name.len == 0 || name.len > (sizeof(path) - 1)) {
		return MGMT_ERR_EINVAL;
	}

	/* Copy path and ensure it is null-teminated */
	memcpy(path, name.value, name.len);
	path[name.len] = '\0';

	/* Retrieve file size */
	rc = lcz_fs_mgmt_impl_filelen(path, &file_len);

	/* Encode the response. */
	if (rc == 0) {
		/* No error, only encode file status (length) */
		ok = zcbor_tstr_put_lit(zse, "len") && zcbor_uint64_put(zse, file_len);
	} else {
		/* Error, only encode error result code */
		ok = zcbor_tstr_put_lit(zse, "rc") && zcbor_int32_put(zse, rc);
	}

	if (!ok) {
		return MGMT_ERR_ENOMEM;
	}

	return 0;
}
#endif

#if defined(CONFIG_LCZ_FS_MGMT_CHECKSUM_HASH)
/**
 * Command handler: fs hash/checksum (read)
 */
static int fs_mgmt_file_hash_checksum(struct mgmt_ctxt *ctxt)
{
	char path[CONFIG_LCZ_FS_MGMT_PATH_SIZE + 1];
	char output[FSU_HASH_SIZE];
	uint64_t len = ULLONG_MAX;
	uint64_t off = 0;
	size_t file_len;
	int rc;
	zcbor_state_t *zse = ctxt->cnbe->zs;
	zcbor_state_t *zsd = ctxt->cnbd->zs;
	bool ok;
	struct zcbor_string type = { 0 };
	struct zcbor_string name = { 0 };
	size_t decoded;

	struct zcbor_map_decode_key_val fs_hash_checksum_decode[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(type, zcbor_tstr_decode, &type),
		ZCBOR_MAP_DECODE_KEY_VAL(name, zcbor_tstr_decode, &name),
		ZCBOR_MAP_DECODE_KEY_VAL(off, zcbor_uint64_decode, &off),
		ZCBOR_MAP_DECODE_KEY_VAL(len, zcbor_uint64_decode, &len),
	};

	ok = zcbor_map_decode_bulk(zsd, fs_hash_checksum_decode,
				   ARRAY_SIZE(fs_hash_checksum_decode), &decoded) == 0;
	if (!ok || name.len == 0 || name.len > (sizeof(path) - 1)) {
		return MGMT_ERR_EINVAL;
	}

	/* Copy strings and ensure they are null-teminated */
	memcpy(path, name.value, name.len);
	path[name.len] = '\0';

	/* Make sure that the requested hash type is sha256 */
	if (strncmp(type.value, "sha256", type.len) != 0) {
		return MGMT_ERR_EINVAL;
	}

	/* Check provided length is valid */
	rc = lcz_fs_mgmt_impl_filelen(path, &file_len);
	if (rc != 0) {
		return MGMT_ERR_ENOENT;
	}

	/* We cannot support non-zero offsets */
	if (off != 0) {
		return MGMT_ERR_EINVAL;
	}

	/* Length can be zero (full length) or a portion of the file's length */
	if (len == 0) {
		len = file_len;
	} else if (len > file_len) {
		return MGMT_ERR_EINVAL;
	}

	/* Calculate the hash */
	rc = lcz_fs_mgmt_impl_hash(path, len, output);

	/* Encode the response */
	if (rc != 0) {
		ok = zcbor_tstr_put_lit(zse, "rc") && zcbor_int32_put(zse, rc);
	} else {
		ok = zcbor_tstr_put_lit(zse, "type") && zcbor_tstr_put_lit(zse, "sha256");

		ok &= zcbor_tstr_put_lit(zse, "off") && zcbor_uint64_put(zse, 0);

		ok &= zcbor_tstr_put_lit(zse, "len") && zcbor_uint64_put(zse, len);

		ok &= zcbor_tstr_put_lit(zse, "output") &&
		      zcbor_bstr_encode_ptr(zse, output, sizeof(output));
	}

	if (!ok) {
		return MGMT_ERR_ENOMEM;
	}

	return MGMT_ERR_EOK;
}
#endif


/**************************************************************************************************/
/* Global Function Definitions                                                                    */
/**************************************************************************************************/
void lcz_fs_mgmt_register_group(void)
{
	mgmt_register_group(&fs_mgmt_group);
}

#ifdef CONFIG_LCZ_FS_MGMT_FILE_ACCESS_HOOK
void lcz_fs_mgmt_register_evt_cb(fs_mgmt_on_evt_cb cb)
{
	fs_evt_cb = cb;
}
#endif
