/**
 * @file LedPwm.h
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LED_PWM_H__
#define __LED_PWM_H__

/* (Remove Empty Sections) */
/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>
#include <stddef.h>
#include <kernel.h>
#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Constants, Macros and Type Definitions                              */
/******************************************************************************/
typedef struct LedPwm
{
    uint8_t redValue;
    uint8_t greenValue;
    uint8_t blueValue;
}rgbLedColor_t;

/******************************************************************************/
/* Global Data Definitions                                                    */
/******************************************************************************/

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief
 *
 * @param
 * @param
 *
 * @retval
 */
bool LedPwm_RGBon(u16_t ledNumber, rgbLedColor_t ledColor, u32_t period, u32_t pulseWidth);
bool LedPwm_on(u16_t ledNumber, u32_t period, u32_t pulseWidth);
bool LedPwm_off(u16_t ledNumber);
void LedPwm_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* __TEMPLATE_H__ */
