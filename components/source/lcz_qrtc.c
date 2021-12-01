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

#if defined(CONFIG_LCZ_QRTC_USE_ERRNO)
#include <errno.h>
#endif

#include "lcz_qrtc.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
struct qrtc {
	bool epoch_was_set;
	uint32_t offset;
	struct k_mutex mutex;
#if CONFIG_LCZ_QRTC_SYNC_INTERVAL_SECONDS != 0
	struct k_work_delayable work;
#endif
};

#define QRTC_OFFSET_MIN -86400
#define QRTC_OFFSET_MAX 86400
/* tm year is 0 for 1900, therefore do not allow a pre-epoch (1970) year */
#define QRTC_TM_MIN_YEAR 70

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
static uint32_t convert_time_to_epoch(time_t time_data);

#if CONFIG_LCZ_QRTC_SYNC_INTERVAL_SECONDS != 0
static void qrtc_sync_handler(struct k_work *dummy);
#endif

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
uint32_t lcz_qrtc_set_epoch(uint32_t epoch)
{
	update_offset(epoch);
	return lcz_qrtc_get_epoch();
}

uint32_t lcz_qrtc_set_epoch_from_tm(struct tm *time_data,
				    int32_t offset_seconds)
{
	time_t raw_time;
	uint32_t epoch;

	if (offset_seconds >= QRTC_OFFSET_MIN &&
	    offset_seconds <= QRTC_OFFSET_MAX &&
	    time_data->tm_year >= QRTC_TM_MIN_YEAR) {
		raw_time = mktime(time_data);
		epoch = convert_time_to_epoch(raw_time);

		/* (local + offset) = UTC */
		if (offset_seconds <= 0 || (uint32_t)offset_seconds <= epoch) {
			epoch -= offset_seconds;
			update_offset(epoch);
#if defined(CONFIG_LCZ_QRTC_USE_ERRNO)
		} else {
			errno = -EINVAL;
#endif
		}
#if defined(CONFIG_LCZ_QRTC_USE_ERRNO)
	} else {
		errno = -EINVAL;
#endif
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
	k_work_init_delayable(&qrtc.work, qrtc_sync_handler);

	k_work_schedule(&qrtc.work,
			K_SECONDS(CONFIG_LCZ_QRTC_SYNC_INTERVAL_SECONDS));
#endif

	return 0;
}

/**
 * @brief Generate and save an offset using the current uptime.
 * This is a quasi-RTC because it isn't battery backed or temperature
 * compensated. Its resolution depends on k_uptime_get.
 */
static void update_offset(uint32_t epoch)
{
	if (epoch < CONFIG_LCZ_QRTC_MINIMUM_EPOCH) {
		LOG_ERR("Invalid epoch time - QRTC offset not updated");
#if defined(CONFIG_LCZ_QRTC_USE_ERRNO)
		errno = -EINVAL;
#endif
	} else {
		k_mutex_lock(&qrtc.mutex, K_FOREVER);
		uint32_t uptime = get_uptime_seconds();
		if (epoch >= uptime) {
			qrtc.offset = epoch - uptime;
			qrtc.epoch_was_set = true;
#if defined(CONFIG_LCZ_QRTC_USE_ERRNO)
		} else {
			errno = -EINVAL;
#endif
		}
		k_mutex_unlock(&qrtc.mutex);
	}
}

static uint32_t get_uptime_seconds(void)
{
	int64_t uptime_ms = k_uptime_get();

	if (uptime_ms < 0) {
		return 0;
	}
	return (uint32_t)(uptime_ms / MSEC_PER_SEC);
}

static uint32_t convert_time_to_epoch(time_t time_data)
{
	/* Time is a long long int in Zephyr. */
	if (time_data < 0) {
		return 0;
	} else if (time_data >= UINT32_MAX) {
		return 0;
	} else {
		return (uint32_t)time_data;
	}
}

#if CONFIG_LCZ_QRTC_SYNC_INTERVAL_SECONDS != 0
static void qrtc_sync_handler(struct k_work *dummy)
{
	ARG_UNUSED(dummy);

	lcz_qrtc_sync_handler();

	k_work_schedule(&qrtc.work,
			K_SECONDS(CONFIG_LCZ_QRTC_SYNC_INTERVAL_SECONDS));
}

__weak void lcz_qrtc_sync_handler(void)
{
	return;
}
#endif

SYS_INIT(qrtc_sys_init, POST_KERNEL, CONFIG_LCZ_QRTC_INIT_PRIORITY);
