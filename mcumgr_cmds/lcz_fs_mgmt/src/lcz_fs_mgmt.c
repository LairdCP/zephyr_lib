/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <limits.h>
#include <string.h>
#include "cborattr/cborattr.h"
#include "mgmt/mgmt.h"
#include "file_system_utilities.h"
#include "lcz_fs_mgmt/fs_mgmt.h"
#include "lcz_fs_mgmt/fs_mgmt_impl.h"
#include "lcz_fs_mgmt/fs_mgmt_config.h"

static int fs_mgmt_file_download(struct mgmt_ctxt *ctxt);
static int fs_mgmt_file_upload(struct mgmt_ctxt *ctxt);
static int fs_mgmt_file_size(struct mgmt_ctxt *ctxt);
static int fs_mgmt_file_crc32(struct mgmt_ctxt *ctxt);
static int fs_mgmt_file_sha256(struct mgmt_ctxt *ctxt);
static struct fs_mgmt_ctxt fs_mgmt_ctxt;

static const struct mgmt_handler fs_mgmt_handlers[] = {
    [FS_MGMT_ID_FILE] = {
        .mh_read = fs_mgmt_file_download,
        .mh_write = fs_mgmt_file_upload,
    },
    [FS_MGMT_ID_SIZE] = {
        .mh_read = fs_mgmt_file_size,
        .mh_write = NULL,
    },
    [FS_MGMT_ID_CRC32] = {
        .mh_read = fs_mgmt_file_crc32,
        .mh_write = NULL,
    },
    [FS_MGMT_ID_SHA256] = {
        .mh_read = fs_mgmt_file_sha256,
        .mh_write = NULL,
    },
};

#define FS_MGMT_HANDLER_CNT \
    (sizeof fs_mgmt_handlers / sizeof fs_mgmt_handlers[0])

static struct mgmt_group fs_mgmt_group = {
    .mg_handlers = fs_mgmt_handlers,
    .mg_handlers_count = FS_MGMT_HANDLER_CNT,
    .mg_group_id = MGMT_GROUP_ID_FS,
};

/**
 * Command handler: fs file (read)
 */
static int
fs_mgmt_file_download(struct mgmt_ctxt *ctxt)
{
    uint8_t file_data[FS_MGMT_DL_CHUNK_SIZE];
    char path[FS_MGMT_PATH_SIZE + 1];
    unsigned long long off;
    CborError err;
    size_t bytes_read;
    size_t file_len;
    int rc;

    const struct cbor_attr_t dload_attr[] = {
        {
            .attribute = "off",
            .type = CborAttrUnsignedIntegerType,
            .addr.uinteger = &off,
        },
        {
            .attribute = "name",
            .type = CborAttrTextStringType,
            .addr.string = path,
            .len = sizeof path,
        },
        { 0 },
    };

    off = ULLONG_MAX;
    rc = cbor_read_object(&ctxt->it, dload_attr);
    if (rc != 0 || off == ULLONG_MAX) {
        return MGMT_ERR_EINVAL;
    }

#if defined(CONFIG_LCZ_FS_MGMT_APPLICATION_ACCESS_CHECK)
    /* Check with the application if access should be granted */
    rc = fs_mgmt_impl_app_access_check(ACCESS_TYPE_READ, path);

    if (rc != 0) {
        /* Access has not been granted, deny */
        return MGMT_ERR_EPERUSER;
    }
#endif

#if defined(CONFIG_LCZ_FS_MGMT_APPLICATION_REDIRECT)
    fs_mgmt_impl_app_redirect(ACCESS_TYPE_READ, path);
#endif

    /* Only the response to the first download request contains the total file
     * length.
     */
    if (off == 0) {
        rc = fs_mgmt_impl_filelen(path, &file_len);
        if (rc != 0) {
            return rc;
        }
    }

    /* Read the requested chunk from the file. */
    rc = fs_mgmt_impl_read(path, off, FS_MGMT_DL_CHUNK_SIZE,
                           file_data, &bytes_read);
    if (rc != 0) {
        return rc;
    }

    /* Encode the response. */
    err = 0;
    err |= cbor_encode_text_stringz(&ctxt->encoder, "off");
    err |= cbor_encode_uint(&ctxt->encoder, off);
    err |= cbor_encode_text_stringz(&ctxt->encoder, "data");
    err |= cbor_encode_byte_string(&ctxt->encoder, file_data, bytes_read);
    err |= cbor_encode_text_stringz(&ctxt->encoder, "rc");
    err |= cbor_encode_int(&ctxt->encoder, MGMT_ERR_EOK);
    if (off == 0) {
        err |= cbor_encode_text_stringz(&ctxt->encoder, "len");
        err |= cbor_encode_uint(&ctxt->encoder, file_len);
    }

    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }

    return 0;
}

/**
 * Encodes a file upload response.
 */
static int
fs_mgmt_file_upload_rsp(struct mgmt_ctxt *ctxt, int rc, unsigned long long off)
{
    CborError err;

    err = 0;
    err |= cbor_encode_text_stringz(&ctxt->encoder, "rc");
    err |= cbor_encode_int(&ctxt->encoder, rc);
    err |= cbor_encode_text_stringz(&ctxt->encoder, "off");
    err |= cbor_encode_uint(&ctxt->encoder, off);

    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }

    return 0;
}

/**
 * Command handler: fs file (write)
 */
static int
fs_mgmt_file_upload(struct mgmt_ctxt *ctxt)
{
    uint8_t file_data[FS_MGMT_UL_CHUNK_SIZE];
    char file_name[FS_MGMT_PATH_SIZE + 1];
    unsigned long long len;
    unsigned long long off;
    size_t data_len;
    size_t new_off;
    int rc;

    const struct cbor_attr_t uload_attr[5] = {
        [0] = {
            .attribute = "off",
            .type = CborAttrUnsignedIntegerType,
            .addr.uinteger = &off,
            .nodefault = true
        },
        [1] = {
            .attribute = "data",
            .type = CborAttrByteStringType,
            .addr.bytestring.data = file_data,
            .addr.bytestring.len = &data_len,
            .len = sizeof(file_data)
        },
        [2] = {
            .attribute = "len",
            .type = CborAttrUnsignedIntegerType,
            .addr.uinteger = &len,
            .nodefault = true
        },
        [3] = {
            .attribute = "name",
            .type = CborAttrTextStringType,
            .addr.string = file_name,
            .len = sizeof(file_name)
        },
        [4] = { 0 },
    };

    len = ULLONG_MAX;
    off = ULLONG_MAX;
    rc = cbor_read_object(&ctxt->it, uload_attr);
    if (rc != 0 || off == ULLONG_MAX || file_name[0] == '\0') {
        return MGMT_ERR_EINVAL;
    }

#if defined(CONFIG_LCZ_FS_MGMT_APPLICATION_ACCESS_CHECK)
    /* Check with the application if access should be granted */
    rc = fs_mgmt_impl_app_access_check(ACCESS_TYPE_WRITE, file_name);

    if (rc != 0) {
        /* Access has not been granted, deny */
        return MGMT_ERR_EPERUSER;
    }
#endif

#if defined(CONFIG_LCZ_FS_MGMT_APPLICATION_REDIRECT)
    fs_mgmt_impl_app_redirect(ACCESS_TYPE_WRITE, file_name);
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
            return fs_mgmt_file_upload_rsp(ctxt, MGMT_ERR_EINVAL,
                                           fs_mgmt_ctxt.off);
        }
    }

    new_off = fs_mgmt_ctxt.off + data_len;
    if (new_off > fs_mgmt_ctxt.len) {
        /* Data exceeds image length. */
        return MGMT_ERR_EINVAL;
    }

    if (data_len > 0) {
#if defined(CONFIG_LCZ_FS_MGMT_INTERCEPT)
        /* Pass to application to decide action */
        rc = fs_mgmt_impl_app_intercept(file_name, &fs_mgmt_ctxt, len, off,
                                        file_data, data_len);
#else
        /* Write the data chunk to the file. */
        rc = fs_mgmt_impl_write(file_name, off, file_data, data_len);
#endif

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
    return fs_mgmt_file_upload_rsp(ctxt, 0, fs_mgmt_ctxt.off);
}

/**
 * Command handler: fs size (read)
 */
static int
fs_mgmt_file_size(struct mgmt_ctxt *ctxt)
{
    CborError err = 0;
    int r = 0;
    char path[FS_MGMT_PATH_SIZE + 1];
    size_t name_length;
    int32_t file_size = 0;

    memset(path, 0, sizeof(path));

    const struct cbor_attr_t params_attr[] = {
        {
            .attribute = "name",
            .type = CborAttrTextStringType,
            .addr.string = path,
            .len = sizeof(path),
            .nodefault = true
        },
	{
            0
        }
    };

    if (cbor_read_object(&ctxt->it, params_attr) != 0) {
        return MGMT_ERR_EINVAL;
    }

#if defined(CONFIG_LCZ_FS_MGMT_APPLICATION_ACCESS_CHECK)
    /* Check with the application if access should be granted */
    r = fs_mgmt_impl_app_access_check(ACCESS_TYPE_READ, path);

    if (r != 0) {
        /* Access has not been granted, deny */
        return MGMT_ERR_EPERUSER;
    }
#endif

#if defined(CONFIG_LCZ_FS_MGMT_APPLICATION_REDIRECT)
    fs_mgmt_impl_app_redirect(ACCESS_TYPE_READ, path);
#endif

    name_length = strlen(path);
    if ((name_length > 0) && (name_length < CONFIG_FSU_MAX_FILE_NAME_SIZE)) {
        file_size = fsu_get_file_size_abs(path);
    }

    err |= cbor_encode_text_stringz(&ctxt->encoder, "r");
    err |= cbor_encode_int(&ctxt->encoder, r);
    err |= cbor_encode_text_stringz(&ctxt->encoder, "s");
    err |= cbor_encode_uint(&ctxt->encoder, file_size);

    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }

    return 0;
}

/**
 * Command handler: fs crc32 (read)
 */
static int
fs_mgmt_file_crc32(struct mgmt_ctxt *ctxt)
{
#ifdef CONFIG_FSU_CHECKSUM
    CborError err = 0;
    int r = 0;
    char path[FS_MGMT_PATH_SIZE + 1];
    uint32_t checkfile_file = 0;
    size_t name_length;
    size_t file_size;

    memset(path, 0, sizeof(path));

    const struct cbor_attr_t params_attr[] = {
        {
            .attribute = "name",
            .type = CborAttrTextStringType,
            .addr.string = path,
            .len = sizeof(path),
            .nodefault = true
        },
	{
            0
        }
    };

    if (cbor_read_object(&ctxt->it, params_attr) != 0) {
        return MGMT_ERR_EINVAL;
    }

#if defined(CONFIG_LCZ_FS_MGMT_APPLICATION_ACCESS_CHECK)
    /* Check with the application if access should be granted */
    r = fs_mgmt_impl_app_access_check(ACCESS_TYPE_READ, path);

    if (r != 0) {
        /* Access has not been granted, deny */
        return MGMT_ERR_EPERUSER;
    }
#endif

#if defined(CONFIG_LCZ_FS_MGMT_APPLICATION_REDIRECT)
    fs_mgmt_impl_app_redirect(ACCESS_TYPE_READ, path);
#endif

    name_length = strlen(path);
    if ((name_length > 0) && (name_length < CONFIG_FSU_MAX_FILE_NAME_SIZE)) {
        file_size = fsu_get_file_size_abs(path);

        if (file_size > 0) {
            r = fsu_crc32_abs(&checkfile_file, path, file_size);
        }
    }

    err |= cbor_encode_text_stringz(&ctxt->encoder, "r");
    err |= cbor_encode_int(&ctxt->encoder, r);
    err |= cbor_encode_text_stringz(&ctxt->encoder, "c");
    err |= cbor_encode_uint(&ctxt->encoder, checkfile_file);

    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }

    return 0;
#else
    return MGMT_ERR_ENOTSUP;
#endif
}

/**
 * Command handler: fs sha256 (read)
 */
static int
fs_mgmt_file_sha256(struct mgmt_ctxt *ctxt)
{
#ifdef CONFIG_FSU_HASH
    CborError err = 0;
    int r = 0;
    char path[FS_MGMT_PATH_SIZE + 1];
    uint8_t hash[FSU_HASH_SIZE] = { 0x00 };
    size_t name_length;
    size_t file_size;

    memset(path, 0, sizeof(path));

    const struct cbor_attr_t params_attr[] = {
        {
            .attribute = "name",
            .type = CborAttrTextStringType,
            .addr.string = path,
            .len = sizeof(path),
            .nodefault = true
        },
	{
            0
        }
    };

    if (cbor_read_object(&ctxt->it, params_attr) != 0) {
        return MGMT_ERR_EINVAL;
    }

#if defined(CONFIG_LCZ_FS_MGMT_APPLICATION_ACCESS_CHECK)
    /* Check with the application if access should be granted */
    r = fs_mgmt_impl_app_access_check(ACCESS_TYPE_READ, path);

    if (r != 0) {
        /* Access has not been granted, deny */
        return MGMT_ERR_EPERUSER;
    }
#endif

#if defined(CONFIG_LCZ_FS_MGMT_APPLICATION_REDIRECT)
    fs_mgmt_impl_app_redirect(ACCESS_TYPE_READ, path);
#endif

    name_length = strlen(path);
    if ((name_length > 0) && (name_length < CONFIG_FSU_MAX_FILE_NAME_SIZE)) {
        file_size = fsu_get_file_size_abs(path);

        if (file_size > 0) {
            r = fsu_sha256_abs(hash, path, file_size);
        }
    }

    err |= cbor_encode_text_stringz(&ctxt->encoder, "r");
    err |= cbor_encode_int(&ctxt->encoder, r);
    err |= cbor_encode_text_stringz(&ctxt->encoder, "h");
    err |= cbor_encode_byte_string(&ctxt->encoder, hash, FSU_HASH_SIZE);

    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }

    return 0;
#else
    return MGMT_ERR_ENOTSUP;
#endif
}

void
lcz_fs_mgmt_register_group(void)
{
    mgmt_register_group(&fs_mgmt_group);
}
