/**
 * @file mcumgr_wrapper.c
 * @brief
 *
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Prevas A/S
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>
LOG_MODULE_REGISTER(mcumgr_wrapper);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr.h>
#include <stats/stats.h>

#ifdef CONFIG_MCUMGR_CMD_FS_MGMT
#include "fs_mgmt/fs_mgmt.h"
#endif
#ifdef CONFIG_FILE_SYSTEM_LITTLEFS
#ifdef CONFIG_FILE_SYSTEM_UTILITIES
#include "file_system_utilities.h"
#endif
#endif
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
#include "os_mgmt/os_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
#include "img_mgmt/img_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_STAT_MGMT
#include "stat_mgmt/stat_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_SMP_BT
#include <mgmt/smp_bt.h>
#endif
#ifdef CONFIG_SIMPLE_BLUETOOTH
#include "simple_bluetooth.h"
#endif

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/

/* Define an example stats group; approximates time since boot. */
STATS_SECT_START(smp_svr_stats)
STATS_SECT_ENTRY(timer_ticks)
STATS_SECT_END;

/* Assign a name to the `ticks` stat. */
STATS_NAME_START(smp_svr_stats)
STATS_NAME(smp_svr_stats, timer_ticks)
STATS_NAME_END(smp_svr_stats);

/* Define an instance of the stats group. */
STATS_SECT_DECL(smp_svr_stats) smp_svr_stats;

#if CONFIG_STATS_TICK_RATE_MS > 0
struct k_timer tick_timer;
#endif

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
#if CONFIG_STATS_TICK_RATE_MS > 0
static void tick_timer_callback_isr(struct k_timer *timer_id);
#endif

/******************************************************************************/
/* Global Functions                                                           */
/******************************************************************************/
void mcumgr_wrapper_register_subsystems(void)
{
	int rc = STATS_INIT_AND_REG(smp_svr_stats, STATS_SIZE_32,
				    "smp_svr_stats");

	if (rc < 0) {
		LOG_ERR("Error initializing stats system [%d]", rc);
	}

	/* Register the built-in mcumgr command handlers. */
#ifdef CONFIG_FILE_SYSTEM_LITTLEFS
#ifdef CONFIG_FILE_SYSTEM_UTILITIES
	fsu_lfs_mount();
#endif
#endif
#ifdef CONFIG_MCUMGR_CMD_FS_MGMT
	fs_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
	os_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
	img_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_CMD_STAT_MGMT
	stat_mgmt_register_group();
#if CONFIG_STATS_TICK_RATE_MS > 0
	k_timer_init(&tick_timer, tick_timer_callback_isr, NULL);
	k_timer_start(&tick_timer, K_MSEC(CONFIG_STATS_TICK_RATE_MS),
		      K_MSEC(CONFIG_STATS_TICK_RATE_MS));
#endif
#endif
#ifdef CONFIG_MCUMGR_SMP_BT
	smp_bt_register();
#if CONFIG_SIMPLE_BLUETOOTH
	simple_bluetooth_init();
#endif
#endif
}

/******************************************************************************/
/* Interrupt Service Routines                                                 */
/******************************************************************************/
#if CONFIG_STATS_TICK_RATE_MS > 0
static void tick_timer_callback_isr(struct k_timer *timer_id)
{
	ARG_UNUSED(timer_id);
	STATS_INC(smp_svr_stats, timer_ticks);
}
#endif
