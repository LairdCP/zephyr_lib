/*
 * Copyright (c) 2020 Laird Connectivity
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/pwm.h>
#include <logging/log.h>
#include "LEDpwm.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(LED_PWM, LOG_LEVEL);

//LED 1
#if DT_NODE_HAS_PROP(DT_ALIAS(led1pwm), pwms) &&                                 \
	DT_PHA_HAS_CELL(DT_ALIAS(led1pwm), pwms, channel)
/* get the defines from dt (based on alias 'led1pwm') */
#define PWM_DRIVER_1 DT_PWMS_LABEL(DT_ALIAS(led1pwm))
#define PWM_CHANNEL_1 DT_PWMS_CHANNEL(DT_ALIAS(led1pwm))
#if DT_PHA_HAS_CELL(DT_ALIAS(led1pwm), pwms, flags)
#define PWM_FLAGS DT_PWMS_FLAGS(DT_ALIAS(led1pwm))
#else
#define PWM_FLAGS 0
#endif
#else
#error "Choose supported PWM driver"
#endif
//LED 2
#if DT_NODE_HAS_PROP(DT_ALIAS(led2pwm), pwms) &&                                 \
	DT_PHA_HAS_CELL(DT_ALIAS(led2pwm), pwms, channel)
/* get the defines from dt (based on alias 'led2pwm') */
#define PWM_DRIVER_2 DT_PWMS_LABEL(DT_ALIAS(led2pwm))
#define PWM_CHANNEL_2 DT_PWMS_CHANNEL(DT_ALIAS(led2pwm))
#if DT_PHA_HAS_CELL(DT_ALIAS(led2pwm), pwms, flags)
#define PWM_FLAGS DT_PWMS_FLAGS(DT_ALIAS(led2pwm))
#else
#define PWM_FLAGS 0
#endif
#else
#error "Choose supported PWM driver"
#endif
//LED 3
#if DT_NODE_HAS_PROP(DT_ALIAS(led3pwm), pwms) &&                                 \
	DT_PHA_HAS_CELL(DT_ALIAS(led3pwm), pwms, channel)
/* get the defines from dt (based on alias 'led3pwm') */
#define PWM_DRIVER_3 DT_PWMS_LABEL(DT_ALIAS(led3pwm))
#define PWM_CHANNEL_3 DT_PWMS_CHANNEL(DT_ALIAS(led3pwm))
#if DT_PHA_HAS_CELL(DT_ALIAS(led3pwm), pwms, flags)
#define PWM_FLAGS DT_PWMS_FLAGS(DT_ALIAS(led3pwm))
#else
#define PWM_FLAGS 0
#endif
#else
#error "Choose supported PWM driver"
#endif
//LED 4
#if DT_NODE_HAS_PROP(DT_ALIAS(led4pwm), pwms) &&                                 \
	DT_PHA_HAS_CELL(DT_ALIAS(led4pwm), pwms, channel)
/* get the defines from dt (based on alias 'led4pwm') */
#define PWM_DRIVER_4 DT_PWMS_LABEL(DT_ALIAS(led4pwm))
#define PWM_CHANNEL_4 DT_PWMS_CHANNEL(DT_ALIAS(led4pwm))
#if DT_PHA_HAS_CELL(DT_ALIAS(led4pwm), pwms, flags)
#define PWM_FLAGS DT_PWMS_FLAGS(DT_ALIAS(led4pwm))
#else
#define PWM_FLAGS 0
#endif
#else
#error "Choose supported PWM driver"
#endif
//LED 5
#if DT_NODE_HAS_PROP(DT_ALIAS(led5pwm), pwms) &&                                 \
	DT_PHA_HAS_CELL(DT_ALIAS(led5pwm), pwms, channel)
/* get the defines from dt (based on alias 'led5pwm') */
#define PWM_DRIVER_5 DT_PWMS_LABEL(DT_ALIAS(led5pwm))
#define PWM_CHANNEL_5 DT_PWMS_CHANNEL(DT_ALIAS(led5pwm))
#if DT_PHA_HAS_CELL(DT_ALIAS(led5pwm), pwms, flags)
#define PWM_FLAGS DT_PWMS_FLAGS(DT_ALIAS(led5pwm))
#else
#define PWM_FLAGS 0
#endif
#else
#error "Choose supported PWM driver"
#endif
//LED 6
#if DT_NODE_HAS_PROP(DT_ALIAS(led6pwm), pwms) &&                                 \
	DT_PHA_HAS_CELL(DT_ALIAS(led6pwm), pwms, channel)
/* get the defines from dt (based on alias 'led6pwm') */
#define PWM_DRIVER_6 DT_PWMS_LABEL(DT_ALIAS(led6pwm))
#define PWM_CHANNEL_6 DT_PWMS_CHANNEL(DT_ALIAS(led6pwm))
#if DT_PHA_HAS_CELL(DT_ALIAS(led6pwm), pwms, flags)
#define PWM_FLAGS DT_PWMS_FLAGS(DT_ALIAS(led6pwm))
#else
#define PWM_FLAGS 0
#endif
#else
#error "Choose supported PWM driver"
#endif
//LED 7
#if DT_NODE_HAS_PROP(DT_ALIAS(led7pwm), pwms) &&                                 \
	DT_PHA_HAS_CELL(DT_ALIAS(led7pwm), pwms, channel)
/* get the defines from dt (based on alias 'led7pwm') */
#define PWM_DRIVER_7 DT_PWMS_LABEL(DT_ALIAS(led7pwm))
#define PWM_CHANNEL_7 DT_PWMS_CHANNEL(DT_ALIAS(led7pwm))
#if DT_PHA_HAS_CELL(DT_ALIAS(led7pwm), pwms, flags)
#define PWM_FLAGS DT_PWMS_FLAGS(DT_ALIAS(led7pwm))
#else
#define PWM_FLAGS 0
#endif
#else
#error "Choose supported PWM driver"
#endif


 
bool LedPwm_RGBon(u16_t ledNumber, rgbLedColor_t ledColor, u32_t period, u32_t pulseWidth)
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
bool LedPwm_on(u16_t ledNumber, u32_t period, u32_t pulseWidth)
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

bool LedPwm_off(u16_t ledNumber)
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

void LedPwm_shutdown(void)
{
	vibe_off();
	/* TODO: add sleep pin state */
}

