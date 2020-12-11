/**
 * @file lcz_pwm_led.h
 * @brief PWM LED Driver
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __PWM_LED_H__
#define __PWM_LED_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Constants, Macros and Type Definitions                              */
/******************************************************************************/
typedef size_t pwm_led_index_t;

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/

/**
 * @brief Turn on LED
 *
 * @param index of PWM
 * @param period in microseconds
 * @param pulse_width i microseconds
 *
 * @note A valid index is determined by DT aliases during compilation.
 * If dts has led1pwm, led2pwm, and led3pwm then valid indices are 0, 1, 2.
 *
 * @retval negative error code, 0 on success
 */
int lcz_pwm_led_on(pwm_led_index_t index, uint32_t period,
		   uint32_t pulse_width);

/**
 * @brief Turn off LED
 *
 * @param index of PWM
 *
 * @retval negative error code, 0 on success
 */
int lcz_pwm_led_off(pwm_led_index_t index);

#ifdef __cplusplus
}
#endif

#endif /* __PWM_LED_H__ */
