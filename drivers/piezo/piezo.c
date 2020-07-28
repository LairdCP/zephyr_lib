/*
 * Copyright (c) 2020 Laird Connectivity
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/pwm.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include "piezo.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(Piezo, LOG_LEVEL);

#if DT_NODE_HAS_PROP(DT_ALIAS(piezo), pwms) &&                                 \
	DT_PHA_HAS_CELL(DT_ALIAS(piezo), pwms, channel)
/* get the defines from dt (based on alias 'piezo') */
#define PWM_DRIVER DT_PWMS_LABEL(DT_ALIAS(piezo))
#define PWM_CHANNEL DT_PWMS_CHANNEL(DT_ALIAS(piezo))
#if DT_PHA_HAS_CELL(DT_ALIAS(piezo), pwms, flags)
#define PWM_FLAGS DT_PWMS_FLAGS(DT_ALIAS(piezo))
#else
#define PWM_FLAGS 0
#endif
#else
#error "Choose supported PWM driver"
#endif

/* The devicetree node identifier for the "piezogpio" alias. */
#define PIEZO_GPIO_NODE DT_ALIAS(piezogpio)

#if DT_NODE_HAS_STATUS(PIEZO_GPIO_NODE, okay)
#define PIEZO_GPIO_LABEL	DT_GPIO_LABEL(PIEZO_GPIO_NODE, gpios)
#define PIEZO_GPIO_PIN	DT_GPIO_PIN(PIEZO_GPIO_NODE, gpios)
#if DT_PHA_HAS_CELL(PIEZO_GPIO_NODE, gpios, flags)
#define PIEZO_GPIO_FLAGS	DT_GPIO_FLAGS(PIEZO_GPIO_NODE, gpios)
#endif
#else
/* A build error here means your board does not support vibegpio */
#error "Unsupported board: vibegpio devicetree alias is not defined"
#define PIEZO_GPIO_LABEL	""
#define PIEZO_GPIO_PIN	0
#endif

#ifndef PIEZO_GPIO_FLAGS
#define PIEZO_GPIO_FLAGS	0
#endif

void piezo_init(void)
{
	/* configure the piezo GPIO for open drain */
	struct device *piezo_gpio_device;
	piezo_gpio_device = device_get_binding(PIEZO_GPIO_LABEL);
	if (piezo_gpio_device == NULL) {
		printk("Error getting piezo_gpio device\n");
		return;
	}
	gpio_pin_configure(piezo_gpio_device, PIEZO_GPIO_PIN, PIEZO_GPIO_FLAGS);

	/* IMPORTANT: PWM pins must be set low prior to PWM being enabled according to nRF52833 datasheet section 6.16.4 */
	gpio_pin_set_raw(piezo_gpio_device, PIEZO_GPIO_PIN, 0);

	struct device *dev_pwm;

	dev_pwm = device_get_binding(PWM_DRIVER);
#if defined(CONFIG_DEVICE_POWER_MANAGEMENT)
	device_set_power_state(dev_pwm, DEVICE_PM_ACTIVE_STATE, NULL, NULL);
#endif
}

bool piezo_on(uint32_t period, uint32_t pulseWidth)
{
	piezo_init();

	struct device *dev_pwm;
	dev_pwm = device_get_binding(PWM_DRIVER);
	if (!dev_pwm) {
		LOG_ERR("Cannot find %s!\n", PWM_DRIVER);
		return false;
	}
	if (pwm_pin_set_usec(dev_pwm, PWM_CHANNEL, period, pulseWidth,
			     PWM_FLAGS)) {
		LOG_ERR("pwm pin set fails\n");
		return false;
	}
	return true;
}

bool piezo_off(void)
{
	return piezo_on(255, 0);
}

void piezo_shutdown(void)
{
	piezo_off();
	/* TODO: add sleep pin state */
	struct device *dev_pwm;

	dev_pwm = device_get_binding(PWM_DRIVER);
#if defined(CONFIG_DEVICE_POWER_MANAGEMENT)
	device_set_power_state(dev_pwm, DEVICE_PM_LOW_POWER_STATE, NULL, NULL);
#endif

	/* disconnect the GPIO */
	struct device *piezo_gpio_device;
	piezo_gpio_device = device_get_binding(PIEZO_GPIO_LABEL);
	if (piezo_gpio_device == NULL) {
		printk("Error getting piezo_gpio device\n");
		return;
	}
	gpio_pin_configure(piezo_gpio_device, PIEZO_GPIO_PIN, GPIO_DISCONNECTED);

}
