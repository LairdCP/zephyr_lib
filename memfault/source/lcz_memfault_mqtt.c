/**
 * @file lcz_memfault_mqtt.c
 * @brief Memfault MQTT transport
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_mflt_mqtt, CONFIG_LCZ_MEMFAULT_LOG_LEVEL);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr.h>
#include <random/rand32.h>
#include "memfault/core/data_packetizer.h"

#include "lcz_memfault.h"

/******************************************************************************/
/* Global Data Definitions                                                    */
/******************************************************************************/

K_MUTEX_DEFINE(publish_data_mutex);

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/

static uint8_t data_buf[CONFIG_LCZ_MEMFAULT_MQTT_DATA_BUF_LENGTH];

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static bool send_mqtt_data(struct mqtt_client *client, char *topic);
static int publish_data(struct mqtt_client *client, uint8_t *data, size_t len,
			char *topic);
static uint16_t rand16_nonzero_get(void);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/

bool lcz_memfault_publish_data(struct mqtt_client *client, char *topic)
{
	bool sent;

	k_mutex_lock(&publish_data_mutex, K_FOREVER);
	sent = send_mqtt_data(client, topic);
	k_mutex_unlock(&publish_data_mutex);

	return sent;
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/

static bool send_mqtt_data(struct mqtt_client *client, char *topic)
{
	int rc;
	size_t data_len;
	bool data_available;

	while (1) {
		data_len = sizeof(data_buf);
		data_available =
			memfault_packetizer_get_chunk(data_buf, &data_len);
		if (!data_available) {
			LOG_DBG("No data to send");
			break;
		}
		LOG_DBG("Send %d bytes to %s", data_len, log_strdup(topic));
		rc = publish_data(client, data_buf, data_len, topic);
		if (rc != 0) {
			LOG_ERR("Could not publish data %d", rc);
			return false;
		}
	}

	return true;
}

static int publish_data(struct mqtt_client *client, uint8_t *data, size_t len,
			char *topic)
{
	struct mqtt_publish_param param;

	param.message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE;
	param.message.topic.topic.utf8 = topic;
	param.message.topic.topic.size = strlen(param.message.topic.topic.utf8);
	param.message.payload.data = data;
	param.message.payload.len = len;
	/* Message ID of zero is reserved as invalid. */
	param.message_id = rand16_nonzero_get();
	param.dup_flag = 0U;
	param.retain_flag = 0U;

	return mqtt_publish(client, &param);
}

static uint16_t rand16_nonzero_get(void)
{
	uint16_t r = 0;
	do {
		r = (uint16_t)sys_rand32_get();
	} while (r == 0);
	return r;
}
