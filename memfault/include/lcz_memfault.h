/**
 * @file lcz_memfault.h
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_MEMFAULT_H__
#define __LCZ_MEMFAULT_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#ifdef CONFIG_LCZ_MEMFAULT
#include "memfault/core/build_info.h"
#include "memfault/core/platform/device_info.h"
#endif
#ifdef CONFIG_LCZ_MEMFAULT_METRICS
#include "memfault/metrics/metrics.h"
#endif

#ifdef CONFIG_LCZ_MEMFAULT_MQTT_TRANSPORT
#include <net/mqtt.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Constants, Macros and Type Definitions                              */
/******************************************************************************/

#ifdef CONFIG_LCZ_MEMFAULT_METRICS
#define MFLT_METRICS_ADD(key, val)                                             \
	do {                                                                   \
		memfault_metrics_heartbeat_add(MEMFAULT_METRICS_KEY(key),      \
					       val);                           \
	} while (false)

#define MFLT_METRICS_SET_UNSIGNED(key, val)                                    \
	do {                                                                   \
		memfault_metrics_heartbeat_set_unsigned(                       \
			MEMFAULT_METRICS_KEY(key), val);                       \
	} while (false)

#define MFLT_METRICS_SET_SIGNED(key, val)                                      \
	do {                                                                   \
		memfault_metrics_heartbeat_set_signed(                         \
			MEMFAULT_METRICS_KEY(key), val);                       \
	} while (false)

#define MFLT_METRICS_TIMER_START(key)                                          \
	do {                                                                   \
		memfault_metrics_heartbeat_timer_start(                        \
			MEMFAULT_METRICS_KEY(key));                            \
	} while (false)

#define MFLT_METRICS_TIMER_STOP(key)                                           \
	do {                                                                   \
		memfault_metrics_heartbeat_timer_stop(                         \
			MEMFAULT_METRICS_KEY(key));                            \
	} while (false)
#else
#define MFLT_METRICS_ADD(key, val)
#define MFLT_METRICS_SET_UNSIGNED(key, val)
#define MFLT_METRICS_SET_SIGNED(key, val)
#define MFLT_METRICS_TIMER_START(key, val)
#define MFLT_METRICS_TIMER_STOP(key, val)
#endif /* CONFIG_LCZ_MEMFAULT_METRICS */

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/

#ifdef CONFIG_LCZ_MEMFAULT_HTTP_TRANSPORT
/**
 * @brief Initialize memfault SDK
 * 
 * @param api_key API key to communicate to Memfault
 * @return 0 on success 
 */
int lcz_memfault_http_init(char *api_key);

/**
 * @brief Post any available data to memfault cloud via HTTPS
 * 
 * @return 0 on success 
 */
int lcz_memfault_post_data();
#endif

#ifdef CONFIG_LCZ_MEMFAULT_MQTT_TRANSPORT
/**
 * @brief Publish any available data to memfault cloud via MQTT
 * 
 * @return true if data was sent 
 */
bool lcz_memfault_publish_data(struct mqtt_client *client, char *topic);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_MEMFAULT_H__ */
