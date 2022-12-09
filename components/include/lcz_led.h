/**
 * @file lcz_led.h
 * @brief On/Off and simple blink patterns for LED.
 *
 * Copyright (c) 2020-2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_LED_H__
#define __LCZ_LED_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Constants, Macros and Type Definitions                              */
/******************************************************************************/
typedef int led_index_t;

/* The on and off times are in system ticks.
 * If the repeat count is 2 the pattern will be displayed 3
 * times (repeated twice).
 */
typedef struct lcz_led_blink_pattern {
	uint32_t on_time;
	uint32_t off_time;
	uint32_t repeat_count;
} lcz_led_blink_pattern_t;

#define REPEAT_INDEFINITELY (0xFFFFFFFF)

typedef struct lcz_led_configuration {
	/* LED index may be enumerated in application LED configuration.
	 * See lcz_led_configuration.template.h for an example.
	 */
	led_index_t index;

#ifdef CONFIG_LCZ_LED_CUSTOM_ON_OFF
	void (*on)(void);
	void (*off)(void);
#else
	/* from device tree */
	const struct device *dev;
	/* from device tree */
	uint32_t pin;
	gpio_flags_t flags;
#endif

} lcz_led_configuration_t;

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Init the LEDs for the board.
 *
 * LEDs should be off after completion.
 *
 * @param pConfig configuration structure
 * @param size size of structure
 */
void lcz_led_init(struct lcz_led_configuration *pConfig, size_t size);

/**
 * @param index is a Valid LED
 * @return int negative error, 0 on success
 */
int lcz_led_turn_on(led_index_t index);

/**
 * @param index is a Valid LED
 * @return int negative error, 0 on success
 */
int lcz_led_turn_off(led_index_t index);

/**
 * @brief Blink the LED based on the pattern.
 *
 * @param index is a Valid LED
 * @param pPattern Blink pattern @ref struct lcz_led_blink_pattern
 * @param force true = blink the LED even if the LED is already on, false = do not blink the LED and if LED is already on, return an error.
 * @return int negative error, 0 on success
 *
 * @note The pattern is copied by the LED driver.
 */
int lcz_led_blink(led_index_t index,
		  struct lcz_led_blink_pattern const *pPattern, bool force);

/**
 * @param param function called in system work queue context
 * when pattern is complete (use NULL to disable).
 * @return int negative error, 0 on success
 */
int lcz_led_register_pattern_complete_function(led_index_t index,
					       void (*function)(void));

/**
 * @param index of LED
 * @return true if a blink pattern is running
 * @return false if it is complete
 */
bool lcz_led_pattern_busy(led_index_t index);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_LED_H__ */
