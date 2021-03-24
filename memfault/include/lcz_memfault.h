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
#define MFLT_METRICS_TIMER_START(key)
#define MFLT_METRICS_TIMER_STOP(key)
#endif /* CONFIG_LCZ_MEMFAULT_METRICS */

#ifdef CONFIG_LCZ_MEMFAULT_HTTP_TRANSPORT
#define LCZ_MEMFAULT_HTTP_INIT lcz_memfault_http_init
#define LCZ_MEMFAULT_POST_DATA lcz_memfault_post_data
#else
#define LCZ_MEMFAULT_HTTP_INIT(...)
#define LCZ_MEMFAULT_POST_DATA(...)
#endif

#ifdef CONFIG_LCZ_MEMFAULT_MQTT_TRANSPORT
#define LCZ_MEMFAULT_PUBLISH_DATA lcz_memfault_publish_data
#define LCZ_MEMFAULT_BUILD_TOPIC lcz_memfault_build_topic
#else
#define LCZ_MEMFAULT_PUBLISH_DATA(...)
#define LCZ_MEMFAULT_BUILD_TOPIC(...)
#endif

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
int lcz_memfault_post_data(void);
#endif

#ifdef CONFIG_LCZ_MEMFAULT_MQTT_TRANSPORT
/**
 * @brief Publish any available data to memfault cloud via MQTT
 *
 * @return true if data was sent
 */
bool lcz_memfault_publish_data(struct mqtt_client *client);

/**
 * @brief Build topic used for publishing using LCZ_MEMFAULT_MQTT_TOPIC as
 * the format string.
 *
 * @return negative error code, 0 on success
 */
int lcz_memfault_build_topic(const char *board, const char *id);

#endif

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_MEMFAULT_H__ */
