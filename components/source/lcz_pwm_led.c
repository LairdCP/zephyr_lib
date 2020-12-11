/**
 * @file lcz_pwm_led.c
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(pwm_led, CONFIG_LCZ_PWM_LED_LOG_LEVEL);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr.h>
#include <drivers/pwm.h>
#include <init.h>

#include "lcz_pwm_led.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define PWM_LED1_NODE DT_ALIAS(led1pwm)
#define PWM_LED2_NODE DT_ALIAS(led2pwm)
#define PWM_LED3_NODE DT_ALIAS(led3pwm)
#define PWM_LED4_NODE DT_ALIAS(led4pwm)
#define PWM_LED5_NODE DT_ALIAS(led5pwm)
#define PWM_LED6_NODE DT_ALIAS(led6pwm)
#define PWM_LED7_NODE DT_ALIAS(led7pwm)
#define PWM_LED8_NODE DT_ALIAS(led8pwm)
#define PWM_LED9_NODE DT_ALIAS(led9pwm)

#define FLAGS_OR_ZERO(node)                                                    \
	COND_CODE_1(DT_PHA_HAS_CELL(node, pwms, flags), (DT_PWMS_FLAGS(node)), \
		    (0))

#define PWM_LED(x)                                                             \
	{                                                                      \
		.label = DT_PWMS_LABEL(PWM_LED##x##_NODE),                     \
		.channel = DT_PWMS_CHANNEL(PWM_LED##x##_NODE),                 \
		.flags = FLAGS_OR_ZERO(PWM_LED##x##_NODE)                      \
	}

#define DT_HAS_PWM_LED_NODE(x) DT_NODE_HAS_STATUS(PWM_LED##x##_NODE, okay)

struct pwm_led_hardware {
	const char *label;
	uint32_t channel;
	pwm_flags_t flags; /* polarity */
};

/* Generate an array for all nodes that match the form led<x>pwm */
const struct pwm_led_hardware PWM_LED_HARDWARE[] = {
#if DT_HAS_PWM_LED_NODE(1)
	PWM_LED(1),
#endif
#if DT_HAS_PWM_LED_NODE(2)
	PWM_LED(2),
#endif
#if DT_HAS_PWM_LED_NODE(3)
	PWM_LED(3),
#endif
#if DT_HAS_PWM_LED_NODE(4)
	PWM_LED(4),
#endif
#if DT_HAS_PWM_LED_NODE(5)
	PWM_LED(5),
#endif
#if DT_HAS_PWM_LED_NODE(6)
	PWM_LED(6),
#endif
#if DT_HAS_PWM_LED_NODE(7)
	PWM_LED(7),
#endif
#if DT_HAS_PWM_LED_NODE(8)
	PWM_LED(8),
#endif
#if DT_HAS_PWM_LED_NODE(9)
	PWM_LED(9)
#endif
};

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static int pwm_led_init_status;

static const struct device *pwd_led_dev[CONFIG_LCZ_PWM_LED_PINS];

BUILD_ASSERT(ARRAY_SIZE(pwd_led_dev) <= ARRAY_SIZE(PWM_LED_HARDWARE),
	     "Not enough PWM hardware");

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static int lcz_pwm_led_init(const struct device *device);

static int set_pwm(pwm_led_index_t index, uint32_t period,
		   uint32_t pulse_width);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
SYS_INIT(lcz_pwm_led_init, POST_KERNEL, 99);

int lcz_pwm_led_on(pwm_led_index_t index, uint32_t period, uint32_t pulse_width)
{
	return set_pwm(index, period, pulse_width);
}

int lcz_pwm_led_off(pwm_led_index_t index)
{
	return set_pwm(index, 0, 0);
}

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
static int lcz_pwm_led_init(const struct device *device)
{
	ARG_UNUSED(device);
	size_t i;
	pwm_led_init_status = 0;
	for (i = 0; i < CONFIG_LCZ_PWM_LED_PINS; i++) {
		pwd_led_dev[i] = device_get_binding(PWM_LED_HARDWARE[i].label);
		if (pwd_led_dev[i] == NULL) {
			pwm_led_init_status = -ENXIO;
			LOG_ERR("Cannot bind %s", PWM_LED_HARDWARE[i].label);
			break;
		}
	}

	LOG_DBG("Init status: %d", pwm_led_init_status);
	return pwm_led_init_status;
}

int set_pwm(pwm_led_index_t index, uint32_t period, uint32_t pulse_width)
{
	int r;
	if (pwm_led_init_status != 0) {
		r = -EPERM;
	} else if (index < CONFIG_LCZ_PWM_LED_PINS) {
		r = pwm_pin_set_usec(pwd_led_dev[index],
				     PWM_LED_HARDWARE[index].channel, period,
				     pulse_width,
				     PWM_LED_HARDWARE[index].flags);
		if (r != 0) {
			LOG_ERR("Cannot set PWM %u output (%d)", index, r);
		}
	} else {
		r = -EINVAL;
		LOG_ERR("Invalid PWM LED index %u", index);
	}
	return r;
}