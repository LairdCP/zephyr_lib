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
#include "memfault/ports/watchdog.h"
#include "memfault/components.h"
#include "memfault/http/root_certs.h"
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

#ifdef CONFIG_LCZ_MEMFAULT
/* Override the warning in the Nordic file (memfault_integration) with a link
 * to Laird Connectivity's instructions.
 */
BUILD_ASSERT(sizeof(CONFIG_MEMFAULT_NCS_PROJECT_KEY) > 1,
	     "Memfault Project Key not configured. Please visit "
	     "https://goto.memfault.com/create-key/pinnacle-100 or disable "
	     "Memfault with CONFIG_LCZ_MEMFAULT=n");
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

#ifdef CONFIG_LCZ_MEMFAULT
#define LCZ_MEMFAULT_WATCHDOG_ENABLE memfault_software_watchdog_enable
#define LCZ_MEMFAULT_WATCHDOG_FEED memfault_software_watchdog_feed
#define LCZ_MEMFAULT_WATCHDOG_UPDATE_TIMEOUT                                   \
	memfault_software_watchdog_update_timeout
#else
#define LCZ_MEMFAULT_WATCHDOG_ENABLE(...) 0
#define LCZ_MEMFAULT_WATCHDOG_FEED(...) 0
#define LCZ_MEMFAULT_WATCHDOG_UPDATE_TIMEOUT(...) 0
#endif

#ifdef CONFIG_MEMFAULT_LOGGING_ENABLE
#define LCZ_MEMFAULT_COLLECT_LOGS memfault_log_trigger_collection
#else
#define LCZ_MEMFAULT_COLLECT_LOGS(...)
#endif

#ifdef CONFIG_LCZ_MEMFAULT
#define LCZ_MEMFAULT_REBOOT_TRACK_FIRMWARE_UPDATE()                            \
	memfault_reboot_tracking_mark_reset_imminent(                          \
		kMfltRebootReason_FirmwareUpdate, NULL)
#else
#define LCZ_MEMFAULT_REBOOT_TRACK_FIRMWARE_UPDATE()
#endif

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/

#ifdef CONFIG_LCZ_MEMFAULT_HTTP_TRANSPORT
/**
 * @brief Initialize memfault SDK
 *
 * @return 0 on success
 */
int lcz_memfault_http_init(void);

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
 * @brief Build topic used for publishing using the format string.
 *
 * @return negative error code, 0 on success
 */
int lcz_memfault_build_topic(const char *format, ...);

#endif

/**
 * @brief Save Memfault data to a file.
 * The file will contain raw Memfault chunk data.
 * Each chunk will have a two byte (LSB) length header before it.
 * For example, the file contents will look like:
 * <chunk_length><chunk_data><chunk_length><chunk_data>...
 * @param abs_path file name to save to
 * @param file_size size of file in bytes
 * @param has_core_dump true if core dump is present, false otherwise
 * @return int negative errno, 0 on success
 */
int lcz_memfault_save_data_to_file(const char *abs_path, size_t *file_size,
				   bool *has_core_dump);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_MEMFAULT_H__ */
