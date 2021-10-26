/**
 * @file lcz_rpmsg.c
 * @brief Remote procedure message passing to another core
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_rpmsg, CONFIG_LCZ_RPMSG_LOG_LEVEL);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <stdio.h>
#include <zephyr.h>
#include <drivers/ipm.h>
#include <ipc/rpmsg_service.h>
#include <string.h>

#include "lcz_rpmsg.h"

#define LCZ_RPMSG_BUFFER_SIZE (CONFIG_LCZ_RPMSG_MAX_MESSAGE_SIZE + 1)

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static bool valid_user_id(int id);
static int lcz_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len,
			   uint32_t src, void *priv);
static int lcz_register_endpoint(const struct device *arg);

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static int lcz_endpoint_id = -1;

static struct {
	atomic_t users;
	lcz_rpmsg_msg_cb_t *msg_handlers[CONFIG_LCZ_RPMSG_MAX_USERS];
	uint8_t msg_components[CONFIG_LCZ_RPMSG_MAX_USERS];
} rpm;

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
bool lcz_rpmsg_register(int *pId, uint8_t component, lcz_rpmsg_msg_cb_t *cb)
{
        if (lcz_endpoint_id < 0) {
		/* Endpoint not setup, no point in continuing */
		return false;
	}

	*pId = (int)atomic_inc(&rpm.users);

	if (valid_user_id(*pId)) {
		rpm.msg_handlers[*pId] = cb;
		rpm.msg_components[*pId] = component;
		return true;
	} else {
		return false;
	}
}

int lcz_rpmsg_send(uint8_t component, const void *data, size_t len)
{
	int rc = -EIO;
	uint8_t buffer[LCZ_RPMSG_BUFFER_SIZE];

	if (lcz_endpoint_id >= 0) {
#if defined(CONFIG_RPMSG_SERVICE_MODE_MASTER)
		if (rpmsg_service_endpoint_is_bound(lcz_endpoint_id)) {
#endif
			buffer[0] = component;
			memcpy(&buffer[1], data, len);
			rc = rpmsg_service_send(lcz_endpoint_id, buffer, len + 1);
#if defined(CONFIG_RPMSG_SERVICE_MODE_MASTER)
		}
#endif
	}

	return rc;
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static int lcz_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len,
			   uint32_t src, void *priv)
{
	bool handled = false;

	/* Only handle messages that contain data */
	if (len > 1) {
		size_t i;
		uint8_t component = ((uint8_t *)data)[0];

		for (i = 0; i < CONFIG_LCZ_RPMSG_MAX_USERS; i++) {
			if (rpm.msg_handlers[i] != NULL) {
				if (rpm.msg_components[i] !=
				    LCZ_RPMSG_COMPONENT_ALL && component !=
				    rpm.msg_components[i]) {
					/* Not intended for this recipient,
					 * skip to the next one
					 */
					continue;
				}

				uint8_t *buffer = ((uint8_t *)data) +
						  sizeof(component);

				handled |= rpm.msg_handlers[i](component,
							       buffer,
							       (len - 1), src,
							       handled);
			}
		}
	}

        return RPMSG_SUCCESS;
}

static bool valid_user_id(int id)
{
	if (id >= 0 && id < CONFIG_LCZ_RPMSG_MAX_USERS) {
		return true;
	} else {
		LOG_ERR("Invalid RPMSG user ID");
		return false;
	}
}

static int lcz_register_endpoint(const struct device *arg)
{
	int rc = 0;
	lcz_endpoint_id = rpmsg_service_register_endpoint("lcz",
							  lcz_endpoint_cb);

	if (lcz_endpoint_id < 0) {
		rc = lcz_endpoint_id;
	}

	return rc;
}

SYS_INIT(lcz_register_endpoint, POST_KERNEL,
	 CONFIG_RPMSG_SERVICE_EP_REG_PRIORITY);
