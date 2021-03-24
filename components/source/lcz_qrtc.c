/**
 * @file lcz_qrtc.c
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_qrtc, CONFIG_LCZ_QRTC_LOG_LEVEL);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <init.h>
#include <zephyr.h>
#include <stdlib.h>

#include "lcz_qrtc.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
struct qrtc {
	bool epoch_was_set;
	uint32_t offset;
	struct k_mutex mutex;
#if CONFIG_LCZ_QRTC_SYNC_INTERVAL_SECONDS != 0
	struct k_delayed_work work;
#endif
};

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static struct qrtc qrtc;

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static int qrtc_sys_init(const struct device *device);
static void update_offset(uint32_t epoch);
static uint32_t get_uptime_seconds(void);
static uint32_t convert_time_to_epoch(time_t Time);

#if CONFIG_LCZ_QRTC_SYNC_INTERVAL_SECONDS != 0
static void qrtc_sync_handler(struct k_work *dummy);
#endif

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
SYS_INIT(qrtc_sys_init, POST_KERNEL, CONFIG_LCZ_QRTC_INIT_PRIORITY);

uint32_t lcz_qrtc_set_epoch(uint32_t epoch)
{
	update_offset(epoch);
	return lcz_qrtc_get_epoch();
}

uint32_t lcz_qrtc_set_epoch_from_tm(struct tm *pTm, int32_t offset_seconds)
{
	time_t rawTime = mktime(pTm);
	uint32_t epoch = convert_time_to_epoch(rawTime);

	/* (local + offset) = UTC */
	if (abs(offset_seconds) < epoch) {
		epoch -= offset_seconds;
		update_offset(epoch);
	}
	return lcz_qrtc_get_epoch();
}

uint32_t lcz_qrtc_get_epoch(void)
{
	return (get_uptime_seconds() + qrtc.offset);
}

bool lcz_qrtc_epoch_was_set(void)
{
	return qrtc.epoch_was_set;
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static int qrtc_sys_init(const struct device *device)
{
	ARG_UNUSED(device);

	k_mutex_init(&qrtc.mutex);
	qrtc.offset = 0;
	qrtc.epoch_was_set = false;
#if CONFIG_LCZ_QRTC_SYNC_INTERVAL_SECONDS != 0
	k_delayed_work_init(&qrtc.work, qrtc_sync_handler);

	k_delayed_work_submit(&qrtc.work,
			      K_SECONDS(CONFIG_LCZ_QRTC_SYNC_INTERVAL_SECONDS));
#endif

	return 0;
}

/**
 * @brief Generate and save an offset using the current uptime.
 * This is a quasi-RTC because it isn't battery backed or temperature compensated.
 * Its resolution depends on k_uptime_get.
 */
static void update_offset(uint32_t epoch)
{
	if (epoch < CONFIG_LCZ_QRTC_MINIMUM_EPOCH) {
		LOG_ERR("Invalid epoch time - QRTC offset not updated");
	} else {
		k_mutex_lock(&qrtc.mutex, K_FOREVER);
		uint32_t uptime = get_uptime_seconds();
		if (epoch >= uptime) {
			qrtc.offset = epoch - uptime;
			qrtc.epoch_was_set = true;
		}
		k_mutex_unlock(&qrtc.mutex);
	}
}

static uint32_t get_uptime_seconds(void)
{
	int64_t uptimeMs = k_uptime_get();

	if (uptimeMs < 0) {
		return 0;
	}
	return (uint32_t)(uptimeMs / MSEC_PER_SEC);
}

static uint32_t convert_time_to_epoch(time_t Time)
{
	/* Time is a long long int in Zephyr. */
	if (Time < 0) {
		return 0;
	} else if (Time >= UINT32_MAX) {
		return 0;
	} else {
		return (uint32_t)Time;
	}
}

#if CONFIG_LCZ_QRTC_SYNC_INTERVAL_SECONDS != 0
static void qrtc_sync_handler(struct k_work *dummy)
{
	ARG_UNUSED(dummy);

	lcz_qrtc_sync_handler();

	k_delayed_work_submit(&qrtc.work,
			      K_SECONDS(CONFIG_LCZ_QRTC_SYNC_INTERVAL_SECONDS));
}

__weak void lcz_qrtc_sync_handler(void)
{
	return;
}
#endif
