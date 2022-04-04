/**
 * @file lcz_ramdisk.c
 * @brief RAMDISK
 *
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(ramdisk, CONFIG_LCZ_RAMDISK_LOG_LEVEL);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <stdio.h>
#include <zephyr.h>
#include <device.h>
#include <fs/fs.h>
#include <fs/littlefs.h>

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(ramfs);

static struct fs_mount_t ramfs_mnt = { .type = FS_LITTLEFS,
				       .fs_data = &ramfs,
				       .storage_dev =
					       (void *)FLASH_AREA_ID(ramfs),
				       .mnt_point =
					       CONFIG_LCZ_RAMDISK_MOUNT_POINT };

static K_MUTEX_DEFINE(ramfs_init_mutex);

static bool ramfs_mounted;

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
int ramfs_mount(void)
{
	int rc = -ENOSYS;

	k_mutex_lock(&ramfs_init_mutex, K_FOREVER);

	if (!ramfs_mounted) {
		rc = fs_mount(&ramfs_mnt);
		if (rc != 0) {
			LOG_ERR("Error mounting ramfs [%d]", rc);
		} else {
			ramfs_mounted = true;
		}

		if (ramfs_mounted) {
			struct fs_statvfs stats;
			int status = fs_statvfs(ramfs_mnt.mnt_point, &stats);
			LOG_INF("RAMDISK mounted to %s",
				CONFIG_LCZ_RAMDISK_MOUNT_POINT);

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

	k_mutex_unlock(&ramfs_init_mutex);

	return rc;
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
#ifdef CONFIG_LCZ_RAMDISK_LFS_MOUNT
static int lcz_ramdisk_initialise(const struct device *device)
{
	ARG_UNUSED(device);
	return ramfs_mount();
}

SYS_INIT(lcz_ramdisk_initialise, APPLICATION, CONFIG_LCZ_RAMDISK_INIT_PRIORITY);
#endif
