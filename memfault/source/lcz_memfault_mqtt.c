/**
 * @file lcz_memfault_mqtt.c
 * @brief Memfault MQTT transport
 *
 * Copyright (c) 2021-2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lcz_mflt_mqtt, CONFIG_LCZ_MEMFAULT_LOG_LEVEL);

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <stdarg.h>
#include <zephyr/zephyr.h>
#include <zephyr/random/rand32.h>
#include <memfault/core/data_packetizer.h>
#include <lcz_mqtt.h>
#include <attr.h>

#include "lcz_memfault.h"

/**************************************************************************************************/
/* Local Data Definitions                                                                         */
/**************************************************************************************************/
static struct {
	bool initialized;
	bool busy;
	struct lcz_mqtt_user agent;
	struct k_sem sem;
} mqtt_memfault;

/**************************************************************************************************/
/* Local Function Prototypes                                                                      */
/**************************************************************************************************/
static void mqtt_memfault_init(void);
static int mqtt_memfault_send_data(char *buf, size_t buf_size, k_timeout_t chunk_timeout);
static void mqtt_memfault_ack_callback(int status);

/**************************************************************************************************/
/* Global Function Definitions                                                                    */
/**************************************************************************************************/
int lcz_memfault_mqtt_publish_data(char *buf, size_t buf_size, k_timeout_t chunk_timeout)
{
	int r = -EAGAIN;

	if (!mqtt_memfault.busy) {
		mqtt_memfault.busy = true;
		r = mqtt_memfault_send_data(buf, buf_size, chunk_timeout);
		mqtt_memfault.busy = false;
	}

	return r;
}

bool lcz_memfault_mqtt_enabled(void)
{
	return attr_get_bool(ATTR_ID_mqtt_memfault_enable);
}

/**************************************************************************************************/
/* Local Function Definitions                                                                     */
/**************************************************************************************************/
static void mqtt_memfault_init(void)
{
	if (!mqtt_memfault.initialized) {
		k_sem_init(&mqtt_memfault.sem, 0, 1);

		mqtt_memfault.agent.ack_callback = mqtt_memfault_ack_callback;
		lcz_mqtt_register_user(&mqtt_memfault.agent);

		mqtt_memfault.initialized = true;
	}
}

static int mqtt_memfault_send_data(char *buf, size_t buf_size, k_timeout_t chunk_timeout)
{
	int rc = 0;
	size_t data_len;
	bool data_available;
	const char *topic;

	if (!attr_get_bool(ATTR_ID_mqtt_memfault_enable)) {
		return -EPERM;
	}

	topic = attr_get_quasi_static(ATTR_ID_mqtt_memfault_topic);
	if (topic == NULL || strlen(topic) == 0) {
		LOG_ERR("Memfault topic must be set to send data using MQTT");
		return -EINVAL;
	}

	mqtt_memfault_init();

	LOG_DBG("Starting...");

	while (1) {
		data_len = buf_size;
		data_available = memfault_packetizer_get_chunk(buf, &data_len);
		if (!data_available) {
			LOG_DBG("No more data to send");
			break;
		}

		rc = lcz_mqtt_send_binary(buf, data_len, topic, &mqtt_memfault.agent);
		if (rc != 0) {
			LOG_ERR("Could not publish Memfault data %d", rc);
			break;
		} else {
			LOG_DBG("Sending %d bytes", data_len);
		}

		rc = k_sem_take(&mqtt_memfault.sem, chunk_timeout);
		if (rc != 0) {
			LOG_ERR("Memfault MQTT timeout: Could not take semaphore %d", rc);
			break;
		}
	}

	LOG_DBG("Done: %d", rc);

	return rc;
}

static void mqtt_memfault_ack_callback(int status)
{
	if (status < 0) {
		LOG_ERR("%s: MQTT Publish (ack) error: %d", __func__, status);
	} else {
		LOG_DBG("%s: MQTT Ack id: %d", __func__, status);
	}

	k_sem_give(&mqtt_memfault.sem);
}