/*
 * Copyright (c) 2018-2022 mcumgr authors
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_FS_MGMT_
#define H_FS_MGMT_

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************/
/* Global Constant, Macro and Type Definitions                                                    */
/**************************************************************************************************/
/**
 * Command IDs for file system management group.
 */
#define FS_MGMT_ID_FILE 0
#define FS_MGMT_ID_STAT 1
#define FS_MGMT_ID_HASH_CHECKSUM 2

#ifdef CONFIG_LCZ_FS_MGMT_FILE_ACCESS_HOOK
/**
 * @brief Function to be called on fs mgmt event.
 *
 * This callback function is used to notify the application about a pending file
 * read/write request and to authorise or deny it.
 *
 * @param path		The path of the file to query.
 * @param write		True if write access is requested, false for read access
 *
 * @return true to allow the operation, false to deny it
 */
typedef bool (*fs_mgmt_on_evt_cb)(const char *path, bool write);
#endif

/**************************************************************************************************/
/* Global Function Prototypes                                                                     */
/**************************************************************************************************/
/**
 * @brief Registers the file system management command handler group.
 */
void lcz_fs_mgmt_register_group(void);

#ifdef CONFIG_LCZ_FS_MGMT_FILE_ACCESS_HOOK
/**
 * @brief Register file read/write access event callback function.
 *
 * @param cb Callback function or NULL to disable.
 */
void lcz_fs_mgmt_register_evt_cb(fs_mgmt_on_evt_cb cb);
#endif

#ifdef __cplusplus
}
#endif

#endif
