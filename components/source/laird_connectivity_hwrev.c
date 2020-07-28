/**
 * @file laird_connectivity_hwrev.c
 * @brief Hardware revision identification.
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#include <logging/log_output.h>
#include <logging/log_ctrl.h>
LOG_MODULE_REGISTER(laird_connectivity_hwrev);

#define HWREV_LOG_DBG(...) LOG_DBG(__VA_ARGS__)
#define HWREV_LOG_ERR(...) LOG_ERR(__VA_ARGS__)

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/

#include <drivers/sensor.h>
#include <kernel.h>
#include <device.h>
#include <logging/log.h>
#include <drivers/gpio.h>
#include "laird_connectivity_hwrev.h"

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/

static int8_t hardware_revison = HWREV_VERSION_UNKNOWN;

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/

static uint32_t hwrev_clear_pull_resistor(struct device *port, gpio_pin_t pin,
					  gpio_flags_t flags);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/

int hwrev_init()
{
	struct device *gpio_port_dev;
	uint32_t ret;

	hardware_revison = 0;

	/* Setup and read state of ID1 pin */
	gpio_port_dev =
		device_get_binding(DT_PROP(DT_PHANDLE_BY_IDX(DT_PATH(ids, id_1),
							     gpios, 0), label));
	if (!gpio_port_dev) {
		hardware_revison = HWREV_VERSION_UNKNOWN;
		HWREV_LOG_ERR("gpio port (%s for ID1) not found!",
			      DT_PROP(DT_PHANDLE_BY_IDX(
					      DT_PATH(ids, id_1), gpios, 0), label));
		return -ENODEV;
	}

	ret = gpio_pin_configure(gpio_port_dev,
				 DT_GPIO_PIN(DT_PATH(ids, id_1), gpios),
				 DT_GPIO_FLAGS(DT_PATH(ids, id_1), gpios)
				 | GPIO_INPUT | GPIO_INT_DISABLE);
	if (ret) {
		hardware_revison = HWREV_VERSION_UNKNOWN;
		HWREV_LOG_ERR("Error configuring ID1 io %d err: %d!",
			      DT_GPIO_PIN(DT_PATH(ids, id_1), gpios), ret);
		return ret;
	}

	if (gpio_pin_get(gpio_port_dev, DT_GPIO_PIN(DT_PATH(ids, id_1), gpios))) {
		hardware_revison |= HWREV_ID1_BIT_VALUE;

		/* Configure GPIO as input without pull resistor to reduce power
		   consumption */
		hwrev_clear_pull_resistor(gpio_port_dev,
					  DT_GPIO_PIN(DT_PATH(ids, id_1), gpios),
					  DT_GPIO_FLAGS(DT_PATH(ids, id_1), gpios));
	}

	/* Setup and read state of ID2 pin */
	gpio_port_dev =
		device_get_binding(DT_PROP(DT_PHANDLE_BY_IDX(DT_PATH(ids, id_2),
							     gpios, 0), label));
	if (!gpio_port_dev) {
		hardware_revison = HWREV_VERSION_UNKNOWN;
		HWREV_LOG_ERR("gpio port (%s for ID2) not found!",
			      DT_PROP(DT_PHANDLE_BY_IDX(
					      DT_PATH(ids, id_2), gpios, 0), label));
		return -ENODEV;
	}

	ret = gpio_pin_configure(gpio_port_dev,
				 DT_GPIO_PIN(DT_PATH(ids, id_2), gpios),
				 DT_GPIO_FLAGS(DT_PATH(ids, id_2), gpios)
				 | GPIO_INPUT | GPIO_INT_DISABLE);
	if (ret) {
		hardware_revison = HWREV_VERSION_UNKNOWN;
		HWREV_LOG_ERR("Error configuring ID2 io %d err: %d!",
			      DT_GPIO_PIN(DT_PATH(ids, id_2), gpios), ret);
		return ret;
	}
	if (gpio_pin_get(gpio_port_dev, DT_GPIO_PIN(DT_PATH(ids, id_2), gpios))) {
		hardware_revison |= HWREV_ID2_BIT_VALUE;

		/* Configure GPIO as input without pull resistor to reduce power
		   consumption */
		hwrev_clear_pull_resistor(gpio_port_dev,
					  DT_GPIO_PIN(DT_PATH(ids, id_2), gpios),
					  DT_GPIO_FLAGS(DT_PATH(ids, id_2), gpios));
	}

	/* Setup and read state of ID3 pin */
	gpio_port_dev =
		device_get_binding(DT_PROP(DT_PHANDLE_BY_IDX(DT_PATH(ids, id_3),
							     gpios, 0), label));
	if (!gpio_port_dev) {
		hardware_revison = HWREV_VERSION_UNKNOWN;
		HWREV_LOG_ERR("gpio port (%s for ID3) not found!",
			      DT_PROP(DT_PHANDLE_BY_IDX(
					      DT_PATH(ids, id_3), gpios, 0), label));
		return -ENODEV;
	}

	ret = gpio_pin_configure(gpio_port_dev,
				 DT_GPIO_PIN(DT_PATH(ids, id_3), gpios),
				 DT_GPIO_FLAGS(DT_PATH(ids, id_3), gpios)
				 | GPIO_INPUT | GPIO_INT_DISABLE);
	if (ret) {
		hardware_revison = HWREV_VERSION_UNKNOWN;
		HWREV_LOG_ERR("Error configuring ID3 io %d err: %d!",
			      DT_GPIO_PIN(DT_PATH(ids, id_3), gpios), ret);
		return ret;
	}
	if (gpio_pin_get(gpio_port_dev, DT_GPIO_PIN(DT_PATH(ids, id_3), gpios))) {
		hardware_revison |= HWREV_ID3_BIT_VALUE;

		/* Configure GPIO as input without pull resistor to reduce power
		   consumption */
		hwrev_clear_pull_resistor(gpio_port_dev,
					  DT_GPIO_PIN(DT_PATH(ids, id_3), gpios),
					  DT_GPIO_FLAGS(DT_PATH(ids, id_3), gpios));
	}

	/* Output verbose revision if enabled */
	HWREV_LOG_DBG("hardware revision: %d", hardware_revison);

	return 0;
}

int8_t hwrev_get_version()
{
	return hardware_revison;
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static uint32_t hwrev_clear_pull_resistor(struct device *port, gpio_pin_t pin,
					  gpio_flags_t flags)
{
	if (flags & GPIO_PULL_UP) {
		// Remove pull-up resistor
		flags ^= GPIO_PULL_UP;
	} else if (flags & GPIO_PULL_DOWN) {
		// Remove pull-down resistor
		flags ^= GPIO_PULL_DOWN;
	}
	flags |= (GPIO_INPUT | GPIO_INT_DISABLE);

	return gpio_pin_configure(port, pin, flags);
}
