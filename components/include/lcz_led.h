/**
 * @file lcz_led.h
 * @brief On/Off and simple blink patterns for LED.
 *
 * Copyright (c) 2020 Laird Connectivity
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

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Constants, Macros and Type Definitions                              */
/******************************************************************************/
typedef size_t led_index_t;

#define LED_ACTIVE_HIGH true
#define LED_ACTIVE_LOW false

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
	/* LED index may be enumerated in lcz_led_configuration.h */
	led_index_t index;

#ifdef CONFIG_LCZ_LED_CUSTOM_ON_OFF
	void (*on)(void);
	void (*off)(void);
#else
	/* from device tree */
	const char *const dev_name;
	/* from device tree */
	uint32_t pin;
	/* polarity.  on_when_high should be true when '1' turns on LED */
	bool on_when_high;
#endif

} lcz_led_configuration_t;

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Init the LEDs for the board.
 *
 * LEDs should be off after completion.
 * Creates mutex used by on/off/blink functions.
 *
 */
void lcz_led_init(struct lcz_led_configuration *pConfig, size_t size);

/**
 * @param index is a Valid LED
 */
void lcz_led_turn_on(led_index_t index);

/**
 * @param index is a Valid LED
 */
void lcz_led_turn_off(led_index_t index);

/**
 * @param index is a Valid LED
 * @param lcz_led_blink_pattern @ref struct lcz_led_blink_pattern
 *
 * @note The pattern is copied by the LED driver.
 */
void lcz_led_blink(led_index_t index,
		   struct lcz_led_blink_pattern const *pPattern);

/**
 * @param param function called in system work queue context
 * when pattern is complete (use NULL to disable).
 */
void lcz_led_register_pattern_complete_function(led_index_t index,
						void (*function)(void));

/**
 * @param retval true if a blink pattern is running, false if it is complete
 */
bool lcz_led_pattern_busy(led_index_t index);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_LED_H__ */
