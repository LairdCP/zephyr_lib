/**
 * @file lcz_settings.c
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_settings, CONFIG_LCZ_SETTINGS_LOG_LEVEL);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <init.h>
#include <fs/fs.h>

#include "file_system_utilities.h"
#include "lcz_settings.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define BREAK_ON_ERROR(x)                                                      \
	if (x < 0) {                                                           \
		break;                                                         \
	}

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static bool settings_ready;
const char settings_path[] =
	CONFIG_FSU_MOUNT_POINT "/" CONFIG_LCZ_SETTINGS_PATH;

BUILD_ASSERT(sizeof(settings_path) <= CONFIG_FSU_MAX_PATH_SIZE,
	     "Settings path too long");

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static int lcz_settings_init(const struct device *device);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
/* Initialize after fs but before application. */
SYS_INIT(lcz_settings_init, POST_KERNEL, 99);

ssize_t lcz_settings_write(char *name, void *data, size_t size)
{
	if (settings_ready) {
		return fsu_write(settings_path, name, data, size);
	} else {
		return -EPERM;
	}
}

ssize_t lcz_settings_read(char *name, void *data, size_t size)
{
	if (settings_ready) {
		return fsu_read(settings_path, name, data, size);
	} else {
		return -EPERM;
	}
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static int lcz_settings_init(const struct device *device)
{
	ARG_UNUSED(device);
	int r = -EPERM;
	do {
		r = fsu_lfs_mount();
		BREAK_ON_ERROR(r);

		r = fsu_mkdir(CONFIG_FSU_MOUNT_POINT, CONFIG_LCZ_SETTINGS_PATH);
		BREAK_ON_ERROR(r);

		settings_ready = true;
		r = 0;
	} while (0);

	LOG_DBG("Init status: %d", r);
	return r;
}