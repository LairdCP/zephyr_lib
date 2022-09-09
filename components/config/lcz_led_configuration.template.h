/**
 * @file lcz_led_configuration.h
 * @brief Example LED configuration file.
 *
 * Copyright (c) 2020-2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_LED_CONFIGURATION_H__
#define __LCZ_LED_CONFIGURATION_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include "lcz_led.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Board definitions                                                          */
/******************************************************************************/

#define LED1_NODE DT_ALIAS(led0)
#define LED2_NODE DT_ALIAS(led1)
#define LED3_NODE DT_ALIAS(led2)
#define LED4_NODE DT_ALIAS(led3)

/* clang-format off */
#define LED1_DEV 	DT_GPIO_LABEL(LED1_NODE, gpios)
#define LED1_FLAGS 	DT_GPIO_FLAGS(LED1_NODE, gpios)
#define LED1 		DT_GPIO_PIN(LED1_NODE, gpios)
#define LED2_DEV	DT_GPIO_LABEL(LED2_NODE, gpios)
#define LED2_FLAGS 	DT_GPIO_FLAGS(LED2_NODE, gpios)
#define LED2 		DT_GPIO_PIN(LED2_NODE, gpios)
#define LED3_DEV 	DT_GPIO_LABEL(LED3_NODE, gpios)
#define LED3_FLAGS 	DT_GPIO_FLAGS(LED3_NODE, gpios)
#define LED3 		DT_GPIO_PIN(LED3_NODE, gpios)
#define LED4_DEV 	DT_GPIO_LABEL(LED4_NODE, gpios)
#define LED4_FLAGS 	DT_GPIO_FLAGS(LED4_NODE, gpios)
#define LED4 		DT_GPIO_PIN(LED4_NODE, gpios)
/* clang-format on */

enum led_index {
	BLUE_LED1 = 0,
	GREEN_LED2,
	RED_LED3,
	GREEN_LED4,
};
BUILD_ASSERT(CONFIG_LCZ_NUMBER_OF_LEDS > GREEN_LED4, "LED object too small");

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_LED_CONFIGURATION_H__ */
