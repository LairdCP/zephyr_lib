/**
 * @file LedPwm.c
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr.h>
#include <device.h>
#include <drivers/pwm.h>
#include <logging/log.h>
#include "LedPwm.h"


/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(LED_PWM, LOG_LEVEL);

#define NUMBER_RGB_COLORS  (4)

#define PWM_LED1_NODE	DT_ALIAS(led1pwm)
#define PWM_LED2_NODE	DT_ALIAS(led2pwm)
#define PWM_LED3_NODE	DT_ALIAS(led3pwm)
#define PWM_LED4_NODE	DT_ALIAS(led4pwm)
#define PWM_LED5_NODE	DT_ALIAS(led5pwm)
#define PWM_LED6_NODE	DT_ALIAS(led6pwm)
#define PWM_LED7_NODE	DT_ALIAS(led7pwm)
#define PWM_LED8_NODE	DT_ALIAS(led8pwm)


#define FLAGS_OR_ZERO(node)						\
	COND_CODE_1(DT_PHA_HAS_CELL(node, pwms, flags),		\
		    (DT_PWMS_FLAGS(node)),				\
		    (0))

//LED 1 
#if DT_NODE_HAS_STATUS(PWM_LED1_NODE, okay)
/* get the defines from dt (based on alias 'led1pwm') */
#define PWM_DRIVER_1 DT_PWMS_LABEL(PWM_LED1_NODE)
#define PWM_CHANNEL_1 DT_PWMS_CHANNEL(PWM_LED1_NODE)
#define PWM_FLAGS_1 FLAGS_OR_ZERO(PWM_LED1_NODE)
#else
#warning "Choose supported PWM driver"
#endif
//LED 2
#if DT_NODE_HAS_STATUS(PWM_LED2_NODE, okay)
/* get the defines from dt (based on alias 'led2pwm') */
#define PWM_DRIVER_2 DT_PWMS_LABEL(PWM_LED2_NODE)
#define PWM_CHANNEL_2 DT_PWMS_CHANNEL(PWM_LED2_NODE)
#define PWM_FLAGS_2 FLAGS_OR_ZERO(PWM_LED2_NODE)
#else
#warning "Choose supported PWM driver"
#endif
//LED 3
#if DT_NODE_HAS_STATUS(PWM_LED3_NODE, okay)
/* get the defines from dt (based on alias 'led3pwm') */
#define PWM_DRIVER_3 DT_PWMS_LABEL(DT_ALIAS(led3pwm))
#define PWM_CHANNEL_3 DT_PWMS_CHANNEL(DT_ALIAS(led3pwm))
#define PWM_FLAGS_3 FLAGS_OR_ZERO(DT_ALIAS(led3pwm))
#else
#warning "Choose supported PWM driver"
#endif
//LED 4
#if DT_NODE_HAS_STATUS(PWM_LED4_NODE, okay)
/* get the defines from dt (based on alias 'led4pwm') */
#define PWM_DRIVER_4 DT_PWMS_LABEL(DT_ALIAS(led4pwm))
#define PWM_CHANNEL_4 DT_PWMS_CHANNEL(DT_ALIAS(led4pwm))
#define PWM_FLAGS_4 FLAGS_OR_ZERO(DT_ALIAS(led4pwm))
#else
#warning "Choose supported PWM driver"
#endif
//LED 5
#if DT_NODE_HAS_STATUS(PWM_LED5_NODE, okay)
/* get the defines from dt (based on alias 'led5pwm') */
#define PWM_DRIVER_5 DT_PWMS_LABEL(DT_ALIAS(led5pwm))
#define PWM_CHANNEL_5 DT_PWMS_CHANNEL(DT_ALIAS(led5pwm))
#define PWM_FLAGS_5 FLAGS_OR_ZERO(DT_ALIAS(led5pwm))
#else
#warning "Choose supported PWM driver"
#endif
//LED 6
#if DT_NODE_HAS_STATUS(PWM_LED6_NODE, okay)
/* get the defines from dt (based on alias 'led6pwm') */
#define PWM_DRIVER_6 DT_PWMS_LABEL(DT_ALIAS(led6pwm))
#define PWM_CHANNEL_6 DT_PWMS_CHANNEL(DT_ALIAS(led6pwm))
#define PWM_FLAGS_6 FLAGS_OR_ZERO(DT_ALIAS(led6pwm))
#else
#warning "Choose supported PWM driver"
#endif
//LED 7
#if DT_NODE_HAS_STATUS(PWM_LED7_NODE, okay)
/* get the defines from dt (based on alias 'led7pwm') */
#define PWM_DRIVER_7 DT_PWMS_LABEL(DT_ALIAS(led7pwm))
#define PWM_CHANNEL_7 DT_PWMS_CHANNEL(DT_ALIAS(led7pwm))
#define PWM_FLAGS_7 FLAGS_OR_ZERO(DT_ALIAS(led7pwm))
#else
#warning "Choose supported PWM driver"
#endif
//LED 7
#if DT_NODE_HAS_STATUS(PWM_LED8_NODE, okay)
/* get the defines from dt (based on alias 'led8pwm') */
#define PWM_DRIVER_8 DT_PWMS_LABEL(DT_ALIAS(led8pwm))
#define PWM_CHANNEL_8 DT_PWMS_CHANNEL(DT_ALIAS(led8pwm))
#define PWM_FLAGS_8 FLAGS_OR_ZERO(DT_ALIAS(led8pwm))
#else
#warning "Choose supported PWM driver"
#endif
/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
struct pwmHardware 
{
	char *driverName;
	uint32_t channelNumber;
	pwm_flags_t pwmFlag;
};

struct pwmHardware ledList[] = 
{
#ifdef PWM_DRIVER_1
	{
		.driverName = PWM_DRIVER_1,
		.channelNumber = PWM_CHANNEL_1,
		.pwmFlag = PWM_FLAGS_1,
	},
#endif
#ifdef PWM_DRIVER_2
	{
		.driverName = PWM_DRIVER_2,
		.channelNumber = PWM_CHANNEL_2,
		.pwmFlag = PWM_FLAGS_2,
	},
#endif
#ifdef PWM_DRIVER_3
	{
		.driverName = PWM_DRIVER_3,
		.channelNumber = PWM_CHANNEL_3,
		.pwmFlag = PWM_FLAGS_3,
	},
#endif
#ifdef PWM_DRIVER_4
	{
		.driverName = PWM_DRIVER_4,
		.channelNumber = PWM_CHANNEL_4,
		.pwmFlag = PWM_FLAGS_4,
	},
#endif
#ifdef PWM_DRIVER_5
	{
		.driverName = PWM_DRIVER_5,
		.channelNumber = PWM_CHANNEL_5,
		.pwmFlag = PWM_FLAGS_5,
	},
#endif
#ifdef PWM_DRIVER_6
	{
		.driverName = PWM_DRIVER_6,
		.channelNumber = PWM_CHANNEL_6,
		.pwmFlag = PWM_FLAGS_6,
	},
#endif
#ifdef PWM_DRIVER_7
	{
		.driverName = PWM_DRIVER_7,
		.channelNumber = PWM_CHANNEL_7,
		.pwmFlag = PWM_FLAGS_7,
	},
#endif
#ifdef PWM_DRIVER_8
	{
		.driverName = PWM_DRIVER_8,
		.channelNumber = PWM_CHANNEL_8,
		.pwmFlag = PWM_FLAGS_8,
	},
#endif
};
	
/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
bool LedPwm_on(uint16_t ledNumber, uint32_t period, uint32_t pulseWidth)
{
	struct device *dev_pwm;
	if(ledNumber > ARRAY_SIZE(ledList))
	{
		LOG_ERR("LED %d is larger than number of LEDs %ld!\n", ledNumber, ARRAY_SIZE(ledList));
		return false;
	}
	dev_pwm = device_get_binding(ledList[ledNumber].driverName);
	if (!dev_pwm) {
		LOG_ERR("Cannot find %s!\n", ledList[ledNumber].driverName);
		return false;
	}
	if (pwm_pin_set_usec(dev_pwm, ledList[ledNumber].channelNumber, period, pulseWidth,
			     ledList[ledNumber].pwmFlag)) {
		LOG_ERR("pwm pin set fails\n");	
		return false;
	}
	return true;
}

bool LedPwm_off(uint16_t ledNumber)
{
	struct device *dev_pwm;
	dev_pwm = device_get_binding(ledList[ledNumber].driverName);
	if (!dev_pwm) 
	{
		LOG_ERR("Cannot find %s!\n", ledList[ledNumber].driverName);
		return false;
	}
	if (pwm_pin_set_usec(dev_pwm, ledList[ledNumber].channelNumber, 0, 0, 
	                     ledList[ledNumber].pwmFlag)) {
		LOG_ERR("pwm pin set fails\n");
		return false;
	}
	return true;
}

void LedPwm_shutdown(void)
{
	uint8_t ledIndex;

	for(ledIndex =0; ledIndex < ARRAY_SIZE(ledList); ledIndex++)
	{
		LedPwm_off(ledIndex);
	}
	/* TODO: add sleep pin state */
}
