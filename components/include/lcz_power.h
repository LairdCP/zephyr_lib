/**
 * @file lcz_power.h
 * @brief Controls power measurement system and software reset.
 *
 * Copyright (c) 2020-2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_POWER_H__
#define __LCZ_POWER_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <devicetree.h>
#include <hal/nrf_power.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Board definitions                                                          */
/******************************************************************************/

/* Default and minimum measurement times between readings */
#define DEFAULT_POWER_TIMER_PERIOD_MS (CONFIG_LCZ_ADC_SAMPLE_PERIOD * 1000)
#define MINIMUM_POWER_TIMER_PERIOD_MS 500

/* Reboot types */
#define REBOOT_TYPE_NORMAL 0
#define REBOOT_TYPE_BOOTLOADER 1

/* ADC0 device */
#define ADC0 DT_PROP(DT_NODELABEL(adc), label)

/* clang-format off */
#define ADC_RESOLUTION               12
#define ADC_ACQUISITION_TIME         ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)
#define ADC_CHANNEL_ID               0
#define ADC_SATURATION               2048
#define ADC_LIMIT_VALUE              4095.0
#define ADC_REFERENCE_VOLTAGE        0.6
#define ADC_DECIMAL_DIVISION_FACTOR  100.0 /* Keeps to 2 decimal places */
#define ADC_GAIN_FACTOR_SIX          6.0
#define ADC_GAIN_FACTOR_TWO          2.0
#define ADC_GAIN_FACTOR_ONE          1.0
#define ADC_GAIN_FACTOR_HALF         0.5
#define GPREGRET_BOOTLOADER_VALUE    0xb1
/* clang-format on */

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Init the power measuring system
 */
void power_init(void);

/**
 * @brief Enables or disables the power measurement system
 *
 * @note Will perform a reading immediately after enabled
 *
 * @param true to enable, false to disable
 */
void power_mode_set(bool enable);

/**
 * @brief Sets the power measurement interval between readings
 *
 * @note Power measuresurement will need to be stopped and started using
 *       power_mode_set() for changes to take effect
 *
 * @param Time in ms (must be >= 500)
 */
void power_interval_set(uint32_t interval_time);

/**
 * @brief Gets the power measurement interval between readings
 *
 * @return Time in ms
 */
uint32_t power_interval_get(void);

/**
 * @brief Enables or disables the power fail comparator
 *
 * @note Once the power fail warning has been sent, internal flash cannot be
 *       written to until the power fail feature is disabled
 *
 * @param true to enable, false to disable
 * @param Power level to trigger the warning on (see nrf_power.h)
 */
void power_fail_set(bool enable, nrf_power_pof_thr_t power_level);

#ifdef CONFIG_REBOOT
/**
 * @brief Reboots the module
 *
 * @param 0 = normal reboot, 1 = stay in UART bootloader
 */
void power_reboot_module(uint8_t type);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_POWER_H__ */
