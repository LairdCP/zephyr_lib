/*
 * Copyright (c) 2020 Laird Connectivity
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/pwm.h>
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

bool piezo_on(uint32_t period, uint32_t pulseWidth)
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

bool piezo_off(void)
{
	struct device *dev_pwm;
	dev_pwm = device_get_binding(PWM_DRIVER);
	if (!dev_pwm) {
		LOG_ERR("Cannot find %s!\n", PWM_DRIVER);
		return false;
	}
	if (pwm_pin_set_usec(dev_pwm, PWM_CHANNEL, 0, 0, PWM_FLAGS)) {
		LOG_ERR("pwm pin set fails\n");
		return false;
	}
	return true;
}

void piezo_shutdown(void)
{
	piezo_off();
	/* TODO: add sleep pin state */
}
