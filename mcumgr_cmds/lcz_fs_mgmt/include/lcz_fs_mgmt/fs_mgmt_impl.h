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

/**
 * @file
 * @brief Declares implementation-specific functions required by file system
 *        management.  The default stubs can be overridden with functions that
 *        are compatible with the host OS.
 */

#ifndef H_FS_MGMT_IMPL_
#define H_FS_MGMT_IMPL_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Access types
 */
typedef enum access_type {
	ACCESS_TYPE_READ = 0,
	ACCESS_TYPE_WRITE,

	ACCESS_TYPE_COUNT
} access_type_t;

#if defined(CONFIG_LCZ_FS_MGMT_INTERCEPT)
/* This file must exist in the application and contain the desired filesystem
 * management structure
 */
#include "lcz_fs_mgmt_intercept.h"
#else
typedef struct fs_mgmt_ctxt {
    /** Whether an upload is currently in progress. */
    bool uploading;

    /** Expected offset of next upload request. */
    size_t off;

    /** Total length of file currently being uploaded. */
    size_t len;
} fs_mgmt_ctxt_t;
#endif

#if defined(CONFIG_LCZ_FS_MGMT_APPLICATION_ACCESS_CHECK)
/**
 * @brief Callback to be implemented in an application when file read/write
 * access is requested (must be implemented in the user application)
 *
 * @param access_type           Type of file access (access_type_t)
 * @param path                  The path of the file which is being accessed.
 *
 * @return                      0 to allow, anything else to deny.
 */
extern int fs_mgmt_impl_app_access_check(uint8_t access_type, const char *path);
#endif

#if defined(CONFIG_LCZ_FS_MGMT_APPLICATION_REDIRECT)
/**
 * @brief Callback to be implemented in an application when file read/write
 * access is allowed but the path can (optionally) be rewritten to something
 * else (must be implemented in the user application)
 *
 * @param access_type           Type of file access (access_type_t)
 * @param path                  The path of the file (to optionally rewrite)
 */
extern void fs_mgmt_impl_app_redirect(uint8_t access_type, char *path);
#endif

#if defined(CONFIG_LCZ_FS_MGMT_INTERCEPT)
/**
 * @brief Callback to be implemented in an application when file write
 * occurs which can then decide what action to take (must be implemented in the
 * user application)
 *
 * @param file_name             Name of file being written
 * @param fs_mgmt_ctxt          Filesystem management context
 * @param len                   Length of file write
 * @param off                   Offset of file write
 * @param file_data             Data being written
 * @param data_len              Size of data
 *
 * @return                      0 on success, MGMT_ERR_[...] code on failure.
 */
extern int fs_mgmt_impl_app_intercept(char *file_name,
				      fs_mgmt_ctxt_t *fs_mgmt_ctxt,
				      unsigned long long len,
				      unsigned long long off,
				      uint8_t *file_data, size_t data_len);
#endif


/**
 * @brief Retrieves the length of the file at the specified path.
 *
 * @param path                  The path of the file to query.
 * @param out_len               On success, the file length gets written here.
 *
 * @return                      0 on success, MGMT_ERR_[...] code on failure.
 */
int fs_mgmt_impl_filelen(const char *path, size_t *out_len);

/**
 * @brief Reads the specified chunk of file data.
 *
 * @param path                  The path of the file to read from.
 * @param offset                The byte offset to read from.
 * @param len                   The number of bytes to read.
 * @param out_data              On success, the file data gets written here.
 * @param out_len               On success, the number of bytes actually read
 *                                  gets written here.
 *
 * @return                      0 on success, MGMT_ERR_[...] code on failure.
 */
int fs_mgmt_impl_read(const char *path, size_t offset, size_t len,
                      void *out_data, size_t *out_len);

/**
 * @brief Writes the specified chunk of file data.  A write to offset 0 must
 * truncate the file; other offsets must append.
 *
 * @param path                  The path of the file to write to.
 * @param offset                The byte offset to write to.
 * @param data                  The data to write to the file.
 * @param len                   The number of bytes to write.
 *
 * @return                      0 on success, MGMT_ERR_[...] code on failure.
 */
int fs_mgmt_impl_write(const char *path, size_t offset, const void *data,
                       size_t len);

#ifdef __cplusplus
}
#endif

#endif
