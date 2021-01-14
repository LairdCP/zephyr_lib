/**
 * @file lcz_memfault.h
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_MEMFAULT_H__
#define __LCZ_MEMFAULT_H__

/* (Remove Empty Sections) */
/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include "memfault/core/build_info.h"
#include "memfault/core/platform/device_info.h"

#ifdef CONFIG_LCZ_MEMFAULT_MQTT_TRANSPORT
#include <net/mqtt.h>
#endif

#ifdef __cplusplus
extern "C" {
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
