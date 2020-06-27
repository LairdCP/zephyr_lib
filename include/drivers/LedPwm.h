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
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Constants, Macros and Type Definitions                              */
/******************************************************************************/
typedef struct LedPwm
{
    uint8_t redDutyValue;
    uint8_t greenDutyValue;
    uint8_t blueDutyValue;
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
bool LedPwm_RGBon(uint16_t ledNumber, rgbLedColor_t ledColor, uint32_t period);
bool LedPwm_on(uint16_t ledNumber, uint32_t period, uint32_t pulseWidth);
bool LedPwm_RGBoff(uint16_t rgbLedNumber);
bool LedPwm_off(uint16_t ledNumber);
void LedPwm_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* __TEMPLATE_H__ */
