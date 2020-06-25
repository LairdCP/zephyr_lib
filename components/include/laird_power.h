/**
 * @file laird_power.h
 * @brief Controls power measurement system and software reset.
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LAIRD_POWER_H__
#define __LAIRD_POWER_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Board definitions                                                          */
/******************************************************************************/

/* Port and pin number of the voltage measurement enabling functionality */
#define MEASURE_ENABLE_PORT DT_NORDIC_NRF_GPIO_0_LABEL
#define MEASURE_ENABLE_PIN 28

/* Measurement time between readings */
#define POWER_TIMER_PERIOD K_MSEC(15000)

/* Reboot types */
#define REBOOT_TYPE_NORMAL 0
#define REBOOT_TYPE_BOOTLOADER 1

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/

/**
 * @brief Init the power measuring system
 */
void power_init(void);

/**
 * @brief Enables or disables the power measurement system
 * @param true to enable, false to disable
 */
void power_mode_set(bool enable);

/**
 * @brief Callback when measurement system is enabled.
 * Override in application or weak implementation will be used.
 */
void power_measurement_callback(uint8_t integer, uint8_t decimal);

#ifdef CONFIG_REBOOT
/**
 * @brief Reboots the module
 * @param 0 = normal reboot, 1 = stay in UART bootloader
 */
void power_reboot_module(uint8_t type);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __POWER_H__ */
