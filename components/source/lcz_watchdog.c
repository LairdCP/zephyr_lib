/**
 * @file lcz_watchdog.c
 * @brief
 *
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2018 Nordic Semiconductor
 * Copyright (c) 2019 Centaur Analytics, Inc
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <logging/log.h>
LOG_MODULE_REGISTER(watchdog, CONFIG_LCZ_WDT_LOG_LEVEL);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr.h>
#include <device.h>
#include <init.h>
#include <drivers/watchdog.h>
#include <logging/log_ctrl.h>

#if defined(LCZ_WDT_MEMFAULT)
#include "lcz_memfault.h"
#endif

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
/*
 * To use this, either the devicetree's /aliases must have a
 * 'watchdog0' property, or one of the following watchdog compatibles
 * must have an enabled node.
 */
#if DT_NODE_HAS_STATUS(DT_ALIAS(watchdog0), okay)
#define WDT_NODE DT_ALIAS(watchdog0)
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_wdt)
#define WDT_NODE DT_INST(0, nordic_nrf_wdt)
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_watchdog)
/* Legacy nRF Connect SDK 1.9 or older */
#define WDT_NODE DT_INST(0, nordic_nrf_watchdog)
#endif

#if defined(WDT_NODE)
#define WDT_DEV_NAME DT_LABEL(WDT_NODE)
#else
#error "Unsupported SoC and no watchdog0 alias in zephyr.dts"
#endif

#define WDT_FEED_RATE_MS (CONFIG_LCZ_WDT_TIMEOUT_MILLISECONDS / 3)

#define WDT_MAX_USERS 31

#define WDT_NOT_INITIALIZED_MSG "WDT module not initialized"

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static K_THREAD_STACK_DEFINE(wdt_workq_stack,
			     CONFIG_LCZ_WDT_WORK_QUEUE_STACK_SIZE);

struct lcz_wdt_obj {
	bool initialized;
	atomic_t users;
	atomic_t check_ins;
	atomic_t check_mask;
	int force_id;
	const struct device *dev;
	int channel_id;
	struct k_work_q work_q;
	struct k_work_delayable feed;
};

static struct lcz_wdt_obj lcz_wdt;

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static int lcz_wdt_initialize(const struct device *device);
static void lcz_wdt_feeder(struct k_work *work);
static bool lcz_wdt_valid_user_id(int id);

static int memfault_wdt_config(void);
static void memfault_wdt_feed(void);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
SYS_INIT(lcz_wdt_initialize, APPLICATION, CONFIG_LCZ_WDT_INIT_PRIORITY);

int lcz_wdt_get_user_id(void)
{
	if (!lcz_wdt.initialized) {
		LOG_ERR(WDT_NOT_INITIALIZED_MSG);
		return -EPERM;
	}

	int id = (int)atomic_inc(&lcz_wdt.users);

	if (lcz_wdt_valid_user_id(id)) {
		return id;
	} else {
		return -EPERM;
	}
}

int lcz_wdt_check_in(int id)
{
	if (!lcz_wdt.initialized) {
		LOG_ERR(WDT_NOT_INITIALIZED_MSG);
		return -EPERM;
	}

	if (lcz_wdt_valid_user_id(id)) {
		atomic_set_bit(&lcz_wdt.check_ins, id);
		atomic_set_bit(&lcz_wdt.check_mask, id);
		return 0;
	} else {
		return -EINVAL;
	}
}

int lcz_wdt_pause(int id)
{
	if (!lcz_wdt.initialized) {
		LOG_ERR(WDT_NOT_INITIALIZED_MSG);
		return -EPERM;
	}

	if (lcz_wdt_valid_user_id(id)) {
		atomic_clear_bit(&lcz_wdt.check_ins, id);
		atomic_clear_bit(&lcz_wdt.check_mask, id);
		return 0;
	} else {
		return -EINVAL;
	}
}

int lcz_wdt_force(void)
{
	if (!lcz_wdt.initialized) {
		LOG_ERR(WDT_NOT_INITIALIZED_MSG);
		return -EPERM;
	}

	LOG_INF("waiting for reset...");
	atomic_set_bit(&lcz_wdt.check_mask, lcz_wdt.force_id);

	return 0;
}

bool lcz_wdt_initialized(void)
{
	return lcz_wdt.initialized;
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static int lcz_wdt_initialize(const struct device *device)
{
	ARG_UNUSED(device);
	struct wdt_timeout_cfg wdt_config;
	int r = 0;
	struct k_work_queue_config cfg = {
		.name = "lcz_wdog",
	};

	LOG_DBG("Initializing watchdog");
	lcz_wdt.dev = device_get_binding(WDT_DEV_NAME);

	lcz_wdt.force_id = atomic_inc(&lcz_wdt.users);

	do {
		if (!lcz_wdt.dev) {
			LOG_ERR("Cannot get WDT device binding: %s",
				WDT_DEV_NAME);
			r = -EPERM;
			break;
		}

		wdt_config.flags = WDT_FLAG_RESET_SOC;
		wdt_config.window.min = 0;
		wdt_config.window.max = CONFIG_LCZ_WDT_TIMEOUT_MILLISECONDS;
		wdt_config.callback = NULL;

		lcz_wdt.channel_id =
			wdt_install_timeout(lcz_wdt.dev, &wdt_config);
		if (lcz_wdt.channel_id < 0) {
			LOG_ERR("Watchdog install error");
			r = -EPERM;
			break;
		}

		k_work_queue_init(&lcz_wdt.work_q);
		k_work_queue_start(&lcz_wdt.work_q, wdt_workq_stack,
				   K_THREAD_STACK_SIZEOF(wdt_workq_stack),
				   K_LOWEST_APPLICATION_THREAD_PRIO, &cfg);

		r = wdt_setup(lcz_wdt.dev, WDT_OPT_PAUSE_HALTED_BY_DBG);
		if (r < 0) {
			LOG_ERR("Watchdog setup error");
			break;
		}

		r = memfault_wdt_config();
		if (r < 0) {
			break;
		}

		k_work_init_delayable(&lcz_wdt.feed, lcz_wdt_feeder);
		r = k_work_schedule_for_queue(&lcz_wdt.work_q, &lcz_wdt.feed,
					      K_NO_WAIT);
		if (r < 0) {
			LOG_ERR("Watchdog feeder init error: %d", r);
			break;
		}

		lcz_wdt.initialized = true;

		LOG_WRN("Watchdog timer started with timeout of %u ms",
			CONFIG_LCZ_WDT_TIMEOUT_MILLISECONDS);
	} while (0);

#if defined(CONFIG_LCZ_WDT_TEST)
	lcz_wdt_force();
#endif

	return r;
}

static void lcz_wdt_feeder(struct k_work *work)
{
	struct k_work_delayable *wd = k_work_delayable_from_work(work);
	struct lcz_wdt_obj *w = CONTAINER_OF(wd, struct lcz_wdt_obj, feed);
	atomic_val_t check_ins = atomic_get(&w->check_ins);
	atomic_val_t check_mask = atomic_get(&w->check_mask);
	int r;
	int r2;

	if (check_ins == check_mask) {
		atomic_clear(&w->check_ins);
		memfault_wdt_feed();
		r = wdt_feed(w->dev, w->channel_id);
	} else {
		r = -EAGAIN;
	}

	if (r < 0) {
		LOG_ERR("Unable to feed watchdog");
	}

	r2 = k_work_reschedule_for_queue(&w->work_q, &w->feed,
					 K_MSEC(WDT_FEED_RATE_MS));
	LOG_DBG("Watchdog feed status: %d reschedule: %d check_ins: %lx mask: %lx",
		r, r2, check_ins, check_mask);
}

static bool lcz_wdt_valid_user_id(int id)
{
	if (id >= 0 && id < WDT_MAX_USERS) {
		return true;
	} else {
		LOG_ERR("Invalid WDT user id");
		return false;
	}
}

static int memfault_wdt_config(void)
{
	int r = 0;

#if defined(LCZ_WDT_MEMFAULT)
	do {
		r = LCZ_MEMFAULT_WATCHDOG_UPDATE_TIMEOUT(
			CONFIG_LCZ_WDT_TIMEOUT_MILLISECONDS -
			CONFIG_LCZ_WDT_MEMFAULT_PRE_FIRE_MS);
		if (r < 0) {
			LOG_ERR("Unable to set memfault software watchdog time");
			break;
		}

		r = LCZ_MEMFAULT_WATCHDOG_ENABLE();
		if (r < 0) {
			LOG_ERR("Unable to enable memfault software watchdog");
			break;
		}
	} while(0);
#endif

	return r;
}

static void memfault_wdt_feed(void)
{
#if defined(LCZ_WDT_MEMFAULT)
	(void)LCZ_MEMFAULT_WATCHDOG_FEED();
#endif
}
