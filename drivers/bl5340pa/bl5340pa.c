/**
 * @file bl5340pa.c
 * @brief Component for getting/setting BL5340PA specific functionality
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(bl5340pa, CONFIG_LCZ_BL5340PA_LOG_LEVEL);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <stdio.h>
#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include <lcz_rpmsg.h>
#include "bl5340pa.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define ANT_SEL_GPIO_LABEL DT_GPIO_LABEL(DT_NODELABEL(nrf_radio_fem), \
						      ant_sel_gpios)
#define ANT_SEL_PIN DT_GPIO_PIN(DT_NODELABEL(nrf_radio_fem), ant_sel_gpios)

#if defined(CONFIG_LIMIT_RADIO_POWER)
#define BL5340PA_RPMSG_RESPONSE_BUFFER_SIZE 16
#else
#define BL5340PA_RPMSG_RESPONSE_BUFFER_SIZE 2
#endif

#define RPMSG_LENGTH_BL5340PA_MIN 1

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static uint8_t module_variant = BL5340PA_VARIANT_INTERNAL_ANTENNA;
static uint8_t antenna_io;
static const struct device *ant_sel_gpio = NULL;
static bool setup_finished = false;
#if defined(CONFIG_LIMIT_RADIO_POWER)
static uint8_t radio_status_complete = false;
static uint8_t radio_status_type;
static uint8_t radio_status_option;
static int32_t radio_status_error;
#endif

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
#if defined(CONFIG_LIMIT_RADIO_POWER)
static void get_status(uint8_t *buffer, size_t len, uint8_t *response,
		       uint8_t *response_length);
#endif

static void get_module_variant(uint8_t *buffer, size_t len, uint8_t *response,
			       uint8_t *response_length);

static void get_active_antenna(uint8_t *buffer, size_t len, uint8_t *response,
			       uint8_t *response_length);

#if defined(CONFIG_LCZ_BL5340PA_ANTENNA_SELECTION)
static void set_active_antenna(uint8_t *buffer, size_t len, uint8_t *response,
			       uint8_t *response_length);
#endif

static bool rpmsg_handler(uint8_t component, void *data, size_t len,
			  uint32_t src, bool handled);

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
#if defined(CONFIG_LIMIT_RADIO_POWER)
static void get_status(uint8_t *buffer, size_t len, uint8_t *response,
		       uint8_t *response_length)
{
	if (len != RPMSG_LENGTH_BL5340PA_GET_STATUS) {
		response[RPMSG_BL5340PA_OFFSET_OPCODE] =
			RPMSG_OPCODE_BL5340PA_INVALID_LENGTH;
		return;
	}

	memset(response, 0, BL5340PA_RPMSG_RESPONSE_BUFFER_SIZE);
	response[RPMSG_BL5340PA_OFFSET_OPCODE] =
		RPMSG_OPCODE_BL5340PA_GET_STATUS;
	*response_length = RPMSG_LENGTH_BL5340PA_GET_STATUS_RESPONSE;

	if (radio_status_complete == true) {
		/* Radio status is known, return the data */
		uint8_t pos = RPMSG_BL5340PA_OFFSET_DATA;
		if (radio_status_error == 0) {
			/* Radio is good to use */
			response[pos] = BL5340PA_RADIO_STATUS_ENABLED;
		} else {
			/* Radio is disabled due to issue with setting
			 * regulatory limit
			 */
			response[pos] = BL5340PA_RADIO_STATUS_DISABLED;
		}

		pos += sizeof(radio_status_complete);
		memcpy(&response[pos], &radio_status_type,
		       sizeof(radio_status_type));
		pos += sizeof(radio_status_type);
		memcpy(&response[pos], &radio_status_option,
		       sizeof(radio_status_option));
		pos += sizeof(radio_status_option);
		memcpy(&response[pos], &radio_status_error,
		       sizeof(radio_status_error));
		pos += sizeof(radio_status_error);
	} else {
		/* Radio status is not yet known, return empty data */
		response[RPMSG_BL5340PA_OFFSET_DATA] =
			BL5340PA_RADIO_STATUS_NOT_KNOWN;
	}
}
#endif

static void get_module_variant(uint8_t *buffer, size_t len, uint8_t *response,
			       uint8_t *response_length)
{
	if (len != RPMSG_LENGTH_BL5340PA_GET_VARIANT) {
		response[RPMSG_BL5340PA_OFFSET_OPCODE] =
			RPMSG_OPCODE_BL5340PA_INVALID_LENGTH;
		return;
	}

	response[RPMSG_BL5340PA_OFFSET_OPCODE] =
		RPMSG_OPCODE_BL5340PA_GET_VARIANT;
	response[RPMSG_BL5340PA_OFFSET_DATA] = module_variant;
	*response_length = RPMSG_LENGTH_BL5340PA_GET_VARIANT_RESPONSE;
}

static void get_active_antenna(uint8_t *buffer, size_t len, uint8_t *response,
			       uint8_t *response_length)
{
	if (len != RPMSG_LENGTH_BL5340PA_GET_ANTENNA) {
		response[RPMSG_BL5340PA_OFFSET_OPCODE] =
			RPMSG_OPCODE_BL5340PA_INVALID_LENGTH;
		return;
	}

	response[RPMSG_BL5340PA_OFFSET_OPCODE] =
		RPMSG_OPCODE_BL5340PA_GET_ANTENNA;
	response[RPMSG_BL5340PA_OFFSET_DATA] = antenna_io;
	*response_length = RPMSG_LENGTH_BL5340PA_GET_ANTENNA_RESPONSE;
}

#if defined(CONFIG_LCZ_BL5340PA_ANTENNA_SELECTION)
static void set_active_antenna(uint8_t *buffer, size_t len, uint8_t *response,
			       uint8_t *response_length)
{
	if (len != RPMSG_LENGTH_BL5340PA_SET_ANTENNA) {
		response[RPMSG_BL5340PA_OFFSET_OPCODE] =
			RPMSG_OPCODE_BL5340PA_INVALID_LENGTH;
		return;
	} else if (ant_sel_gpio == NULL) {
		/* Error getting antenna selection GPIO instance, cannot
		 * continue without it
		 */
		response[RPMSG_BL5340PA_OFFSET_OPCODE] =
			RPMSG_OPCODE_BL5340PA_INTERNAL_ERROR;
		return;
	} else if (module_variant == BL5340PA_VARIANT_INTERNAL_ANTENNA) {
		/* Cannot switch antenna on internal-only variant */
		response[RPMSG_BL5340PA_OFFSET_OPCODE] =
			RPMSG_OPCODE_BL5340PA_WRONG_MODULE;
		return;
	}

	/* External antenna variant, switch to desired antenna */
	uint8_t antenna = buffer[RPMSG_BL5340PA_OFFSET_DATA];
	if (antenna == BL5340PA_ANTENNA_PIN_INTERNAL_ANTENNA ||
	    antenna == BL5340PA_ANTENNA_PIN_EXTERNAL_ANTENNA) {
		int rc = gpio_pin_set(ant_sel_gpio, ANT_SEL_PIN, antenna);

		if (rc == 0) {
			antenna_io = antenna;
			response[RPMSG_BL5340PA_OFFSET_OPCODE] =
				RPMSG_OPCODE_BL5340PA_SET_ANTENNA;
			response[RPMSG_BL5340PA_OFFSET_DATA] = antenna;
			*response_length =
				RPMSG_LENGTH_BL5340PA_SET_ANTENNA_RESPONSE;
		} else {
			response[RPMSG_BL5340PA_OFFSET_OPCODE] =
				RPMSG_OPCODE_BL5340PA_IO_ERROR;
		}
	} else {
		/* Invalid antenna specified */
		response[RPMSG_BL5340PA_OFFSET_OPCODE] =
			RPMSG_OPCODE_BL5340PA_INVALID_ANTENNA;
	}
}
#endif

static bool rpmsg_handler(uint8_t component, void *data, size_t len,
			  uint32_t src, bool handled)
{
	uint8_t response_buffer[BL5340PA_RPMSG_RESPONSE_BUFFER_SIZE];
	uint8_t response_length = RPMSG_LENGTH_BL5340PA_ERROR;

	if (component == RPMSG_COMPONENT_BL5340PA) {
		if (setup_finished == false) {
			/* Module setup did not finish, cannot continue */
			response_buffer[RPMSG_BL5340PA_OFFSET_OPCODE] =
				RPMSG_OPCODE_BL5340PA_INTERNAL_ERROR;
		} else if (len >= RPMSG_LENGTH_BL5340PA_MIN) {
			/* Check which opcode was received */
			uint8_t *buffer = (uint8_t *)data;
			uint8_t opcode = buffer[
				RPMSG_BL5340PA_OFFSET_OPCODE];

			if (opcode == RPMSG_OPCODE_BL5340PA_GET_VARIANT) {
				/* Check which module type this is */
				get_module_variant(buffer, len, response_buffer,
						   &response_length);
			} else if (opcode == RPMSG_OPCODE_BL5340PA_GET_ANTENNA) {
				/* Get active antenna */
				get_active_antenna(buffer, len, response_buffer,
						   &response_length);
#if defined(CONFIG_LIMIT_RADIO_POWER)
			} else if (opcode == RPMSG_OPCODE_BL5340PA_GET_STATUS) {
				/* Check operational status of network core */
				get_status(buffer, len, response_buffer,
					   &response_length);
#endif
#if defined(CONFIG_LCZ_BL5340PA_ANTENNA_SELECTION)
			} else if (opcode == RPMSG_OPCODE_BL5340PA_SET_ANTENNA) {
				/* Set the active antenna */
				set_active_antenna(buffer, len, response_buffer,
						   &response_length);
#endif
			} else {
				/* Unknown opcode */
				response_buffer[RPMSG_BL5340PA_OFFSET_OPCODE] =
					RPMSG_OPCODE_BL5340PA_INVALID_OPCODE;
			}
		} else {
			/* Received packet does not contain at least 1 byte of
			 * data, so there is nothing to process
			 */
			response_buffer[RPMSG_BL5340PA_OFFSET_OPCODE] =
				RPMSG_OPCODE_BL5340PA_INVALID_LENGTH;
		}

		lcz_rpmsg_send(RPMSG_COMPONENT_BL5340PA, response_buffer,
			       response_length);

		return true;
	} else {
		return false;
	}
}

static int bl5340pa_antenna_setup(const struct device *arg)
{
	int rpmsg_id;
	int rc = 0;

	lcz_rpmsg_register(&rpmsg_id, RPMSG_COMPONENT_BL5340PA, rpmsg_handler);

	ant_sel_gpio = device_get_binding(ANT_SEL_GPIO_LABEL);
	if (!ant_sel_gpio) {
		LOG_ERR("Cannot find %s!\n", ANT_SEL_GPIO_LABEL);
		return -EIO;
	}

	rc = gpio_pin_configure(ant_sel_gpio, ANT_SEL_PIN,
				(GPIO_INPUT | GPIO_ACTIVE_HIGH));

	if (rc == 0) {
		antenna_io = (uint8_t)gpio_pin_get(ant_sel_gpio, ANT_SEL_PIN);

		if (antenna_io == BL5340PA_ANTENNA_PIN_EXTERNAL_ANTENNA) {
			module_variant = BL5340PA_VARIANT_EXTERNAL_ANTENNA;

#if defined(CONFIG_LCZ_BL5340PA_ANTENNA_SELECTION)
			/* External antenna, reconfigure antenna select pin as
			 * an output to allow control of the antenna
			 */
			rc = gpio_pin_configure(ant_sel_gpio, ANT_SEL_PIN,
						(GPIO_OUTPUT | GPIO_ACTIVE_HIGH));

			if (rc != 0) {
				LOG_ERR("Failed to configure antenna output: %d\n",
					rc);
				return rc;
			}

			rc = gpio_pin_set(ant_sel_gpio, ANT_SEL_PIN, antenna_io);

			if (rc != 0) {
				LOG_ERR("Failed to set antenna output: %d\n",
					rc);
			}
#endif
		} else {
			module_variant = BL5340PA_VARIANT_INTERNAL_ANTENNA;
		}
	} else {
		LOG_ERR("Failed to configure antenna pin: %d\n", rc);
	}

	if (rc == 0) {
		/* Mark that the component can be used */
		setup_finished = true;
	}

	return rc;
}

#if defined(CONFIG_LIMIT_RADIO_POWER)
void bl5340pa_regulatory_error(uint8_t type, uint8_t option, int32_t error)
{
	radio_status_type = type;
	radio_status_option = option;
	radio_status_error = error;
	radio_status_complete = true;
}
#endif

SYS_INIT(bl5340pa_antenna_setup, POST_KERNEL,
	 CONFIG_RPMSG_SERVICE_EP_REG_PRIORITY);
