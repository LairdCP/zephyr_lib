/**
 * @file lcz_memfault_metrics.c
 * @brief Memfault metrics tracking
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_mflt_metrics, CONFIG_LCZ_MEMFAULT_LOG_LEVEL);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr.h>

#include "lcz_memfault.h"
#include "memfault/core/platform/core.h"
#include "memfault/metrics/platform/timer.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/

static void metrics_timer_work_handler(struct k_work *work);
static void metrics_timer_handler(struct k_timer *dummy);

/******************************************************************************/
/* Global Data Definitions                                                    */
/******************************************************************************/

K_WORK_DEFINE(metrics_timer_work, metrics_timer_work_handler);
K_TIMER_DEFINE(metrics_timer, metrics_timer_handler, NULL);

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/

static MemfaultPlatformTimerCallback *metrics_timer_callback;

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/

bool memfault_platform_metrics_timer_boot(
	uint32_t period_sec, MemfaultPlatformTimerCallback callback)
{
	metrics_timer_callback = callback;
	k_timer_start(&metrics_timer, K_SECONDS(period_sec),
		      K_SECONDS(period_sec));
	return true; /* indicates setup was successful */
}

uint64_t memfault_platform_get_time_since_boot_ms(void)
{
	return k_uptime_get();
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/

static void metrics_timer_work_handler(struct k_work *work)
{
	if (metrics_timer_callback != NULL) {
		metrics_timer_callback();
	}
}

/******************************************************************************/
/* Interrupt Service Routines                                                 */
/******************************************************************************/

static void metrics_timer_handler(struct k_timer *dummy)
{
	k_work_submit(&metrics_timer_work);
}
