/*
 * Copyright (c) 2020 Laird Connectivity
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/pwm.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include "vibe.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(Vibe, LOG_LEVEL);

#if DT_NODE_HAS_PROP(DT_ALIAS(vibemotor), pwms) &&                             \
	DT_PHA_HAS_CELL(DT_ALIAS(vibemotor), pwms, channel)
/* get the defines from dt (based on alias 'vibemotor') */
#define PWM_DRIVER DT_PWMS_LABEL(DT_ALIAS(vibemotor))
#define PWM_CHANNEL DT_PWMS_CHANNEL(DT_ALIAS(vibemotor))
#if DT_PHA_HAS_CELL(DT_ALIAS(vibemotor), pwms, flags)
#define PWM_FLAGS DT_PWMS_FLAGS(DT_ALIAS(vibemotor))
#else
#define PWM_FLAGS 0
#endif
#else
#error "Choose supported PWM driver"
#endif

/* The devicetree node identifier for the "vibegpio" alias. */
#define VIBE_GPIO_NODE DT_ALIAS(vibegpio)

#if DT_NODE_HAS_STATUS(VIBE_GPIO_NODE, okay)
#define VIBE_GPIO_LABEL	DT_GPIO_LABEL(VIBE_GPIO_NODE, gpios)
#define VIBE_GPIO_PIN	DT_GPIO_PIN(VIBE_GPIO_NODE, gpios)
#if DT_PHA_HAS_CELL(VIBE_GPIO_NODE, gpios, flags)
#define VIBE_GPIO_FLAGS	DT_GPIO_FLAGS(VIBE_GPIO_NODE, gpios)
#endif
#else
/* A build error here means your board does not support vibegpio */
#error "Unsupported board: vibegpio devicetree alias is not defined"
#define VIBE_GPIO_LABEL	""
#define VIBE_GPIO_PIN	0
#endif

#ifndef VIBE_GPIO_FLAGS
#define VIBE_GPIO_FLAGS	0
#endif

void vibe_init(void)
{
	/* configure the vibe for low power mode */
	struct device *vibe_gpio_device;
	vibe_gpio_device = device_get_binding(VIBE_GPIO_LABEL);
	if (vibe_gpio_device == NULL) {
		printk("Error getting vibe_gpio device\n");
		return;
	}
	gpio_pin_configure(vibe_gpio_device, VIBE_GPIO_PIN, VIBE_GPIO_FLAGS);

	/* IMPORTANT: PWM pins must be set low prior to PWM being enabled according to nRF52833 datasheet section 6.16.4 */
	gpio_pin_set_raw(vibe_gpio_device, VIBE_GPIO_PIN, 0);

	struct device *dev_pwm;

	dev_pwm = device_get_binding(PWM_DRIVER);
	device_set_power_state(dev_pwm, DEVICE_PM_ACTIVE_STATE, NULL, NULL);
}

bool vibe_on(uint32_t period, uint32_t pulseWidth)
{
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

bool vibe_off(void)
{
	struct device *dev_pwm;
	dev_pwm = device_get_binding(PWM_DRIVER);
	if (!dev_pwm) {
		LOG_ERR("Cannot find %s!\n", PWM_DRIVER);
		return false;
	}
	if (pwm_pin_set_usec(dev_pwm, PWM_CHANNEL, 256, 0, PWM_FLAGS)) {
		LOG_ERR("pwm pin set fails\n");
		return false;
	}
	return true;
}

void vibe_shutdown(void)
{
	vibe_off();
	/* TODO: add sleep pin state */
	struct device *dev_pwm;

	dev_pwm = device_get_binding(PWM_DRIVER);
	device_set_power_state(dev_pwm, DEVICE_PM_LOW_POWER_STATE, NULL, NULL);
}
