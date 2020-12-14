/**
 * @file lcz_led.c
 * @brief LED control
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_led, CONFIG_LCZ_LED_LOG_LEVEL);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <drivers/gpio.h>
#include <kernel.h>

#include "lcz_led.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define TAKE_MUTEX(m) k_mutex_lock(&m, K_FOREVER)
#define GIVE_MUTEX(m) k_mutex_unlock(&m)

#define MINIMUM_ON_TIME_MSEC 1
#define MINIMUM_OFF_TIME_MSEC 1

enum led_state {
	ON = true,
	OFF = false,
};

enum led_blink_state {
	BLINK = true,
	DONT_BLINK = false,
};

struct led {
	enum led_state state;
#ifdef CONFIG_LCZ_LED_CUSTOM_ON_OFF
	void (*on)(void);
	void (*off)(void);
#else
	const struct device *device_handle;
	uint32_t pin;
	bool on_when_high;
#endif
	bool pattern_busy;
	struct lcz_led_blink_pattern pattern;
	struct k_timer timer;
	struct k_work work;
	void (*pattern_complete_function)(void);
};

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static struct k_mutex led_mutex;
static struct led led[CONFIG_LCZ_NUMBER_OF_LEDS];

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static void led_timer_callback(struct k_timer *timer_id);
static void system_workq_led_timer_handler(struct k_work *item);
static void turn_on(struct led *pLed);
static void turn_off(struct led *pLed);
static void change_state(struct led *pLed, bool state, bool blink);

#ifndef CONFIG_LCZ_LED_CUSTOM_ON_OFF
static void led_bind_and_configure(struct lcz_led_configuration *pConfig);
static void led_bind_device(led_index_t index, const char *name);
static void led_configure_pin(led_index_t index, uint32_t pin);
#endif

static bool valid_index(led_index_t index);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void lcz_led_init(struct lcz_led_configuration *pConfig, size_t size)
{
	size_t i;
	struct lcz_led_configuration *pc = pConfig;

	k_mutex_init(&led_mutex);
	TAKE_MUTEX(led_mutex);

	for (i = 0; i < MIN(size, CONFIG_LCZ_NUMBER_OF_LEDS); i++) {
		k_timer_init(&led[i].timer, led_timer_callback, NULL);
		k_timer_user_data_set(&led[i].timer, &led[i]);
		k_work_init(&led[i].work, system_workq_led_timer_handler);
#ifdef CONFIG_LCZ_LED_CUSTOM_ON_OFF
		led[i].on = pc->on;
		led[i].off = pc->off;
		turn_off(&led[i]);
#else
		led_bind_and_configure(pc);
#endif
		pc += 1;
	}
	GIVE_MUTEX(led_mutex);
}

void lcz_led_turn_on(led_index_t index)
{
	if (!valid_index(index)) {
		return;
	}
	TAKE_MUTEX(led_mutex);
	change_state(&led[index], ON, DONT_BLINK);
	GIVE_MUTEX(led_mutex);
}

void lcz_led_turn_off(led_index_t index)
{
	if (!valid_index(index)) {
		return;
	}
	TAKE_MUTEX(led_mutex);
	change_state(&led[index], OFF, DONT_BLINK);
	GIVE_MUTEX(led_mutex);
}

void lcz_led_blink(led_index_t index,
		   struct lcz_led_blink_pattern const *pPattern)
{
	if (!valid_index(index)) {
		return;
	}
	if (pPattern != NULL) {
		TAKE_MUTEX(led_mutex);
		led[index].pattern_busy = true;
		memcpy(&led[index].pattern, pPattern,
		       sizeof(struct lcz_led_blink_pattern));
		led[index].pattern.on_time =
			MAX(led[index].pattern.on_time, MINIMUM_ON_TIME_MSEC);
		led[index].pattern.off_time =
			MAX(led[index].pattern.off_time, MINIMUM_OFF_TIME_MSEC);
		change_state(&led[index], ON, BLINK);
		GIVE_MUTEX(led_mutex);
	} else {
		__ASSERT(false, "NULL LED pattern");
	}
}

void lcz_led_register_pattern_complete_function(led_index_t index,
						void (*function)(void))
{
	if (!valid_index(index)) {
		return;
	}
	TAKE_MUTEX(led_mutex);
	led[index].pattern_complete_function = function;
	GIVE_MUTEX(led_mutex);
}

bool lcz_led_pattern_busy(led_index_t index)
{
	bool result = false;
	if (!valid_index(index)) {
		return false;
	}
	TAKE_MUTEX(led_mutex);
	result = led[index].pattern_busy;
	GIVE_MUTEX(led_mutex);
	return result;
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
#ifndef CONFIG_LCZ_LED_CUSTOM_ON_OFF
static void led_bind_and_configure(struct lcz_led_configuration *pConfig)
{
	if (pConfig->index >= CONFIG_LCZ_NUMBER_OF_LEDS) {
		__ASSERT(false, "Invalid LED Index");
		return;
	}
	led_bind_device(pConfig->index, pConfig->dev_name);
	led[pConfig->index].on_when_high = pConfig->on_when_high;
	led_configure_pin(pConfig->index, pConfig->pin);
}

static void led_bind_device(led_index_t index, const char *name)
{
	led[index].device_handle = device_get_binding(name);
	if (!led[index].device_handle) {
		LOG_ERR("Cannot find %s!", name);
	}
}

static void led_configure_pin(led_index_t index, uint32_t pin)
{
	int ret;
	led[index].pin = pin;
	ret = gpio_pin_configure(led[index].device_handle, led[index].pin,
				 (GPIO_OUTPUT));
	if (ret) {
		LOG_ERR("Error configuring GPIO");
	}
	ret = gpio_pin_set(led[index].device_handle, led[index].pin,
			   led[index].on_when_high ? 0 : 1);
	if (ret) {
		LOG_ERR("Error setting GPIO state");
	}
}
#endif

static void system_workq_led_timer_handler(struct k_work *item)
{
	TAKE_MUTEX(led_mutex);
	struct led *pLed = CONTAINER_OF(item, struct led, work);
	if (pLed->pattern.repeat_count == 0) {
		change_state(pLed, OFF, DONT_BLINK);
		if (pLed->pattern_complete_function != NULL) {
			pLed->pattern_busy = false;
			pLed->pattern_complete_function();
		}
	} else {
		/* Blink patterns start with the LED on, so check the repeat count
		 * after the first on->off cycle has completed (when the repeat
		 * count is non-zero). */
		if (pLed->state == ON) {
			change_state(pLed, OFF, BLINK);
		} else {
			if (pLed->pattern.repeat_count != REPEAT_INDEFINITELY) {
				pLed->pattern.repeat_count -= 1;
			}
			change_state(pLed, ON, BLINK);
		}
	}
	GIVE_MUTEX(led_mutex);
}

static void change_state(struct led *pLed, bool state, bool blink)
{
	if (state == ON) {
		pLed->state = ON;
		turn_on(pLed);
	} else {
		pLed->state = OFF;
		turn_off(pLed);
	}

	if (!blink) {
		pLed->pattern.repeat_count = 0;
		k_timer_stop(&pLed->timer);
	} else {
		if (state == ON) {
			k_timer_start(&pLed->timer,
				      K_MSEC(pLed->pattern.on_time), K_NO_WAIT);
		} else {
			k_timer_start(&pLed->timer,
				      K_MSEC(pLed->pattern.off_time),
				      K_NO_WAIT);
		}
	}

	LOG_DBG("%s %s", state ? "On" : "Off", blink ? "blink" : "Don't blink");
}

static void turn_on(struct led *pLed)
{
#ifdef CONFIG_LCZ_LED_CUSTOM_ON_OFF
	if (pLed->on != NULL) {
		pLed->on();
	}
#else
	gpio_pin_set(pLed->device_handle, pLed->pin,
		     pLed->on_when_high ? 1 : 0);
#endif
}

static void turn_off(struct led *pLed)
{
#ifdef CONFIG_LCZ_LED_CUSTOM_ON_OFF
	if (pLed->off != NULL) {
		pLed->off();
	}
#else
	gpio_pin_set(pLed->device_handle, pLed->pin,
		     pLed->on_when_high ? 0 : 1);
#endif
}

static bool valid_index(led_index_t index)
{
	if (index < CONFIG_LCZ_NUMBER_OF_LEDS) {
		return true;
	} else {
		__ASSERT(false, "Invalid LED Index");
		return false;
	}
}

/******************************************************************************/
/* Interrupt Service Routines                                                 */
/******************************************************************************/
static void led_timer_callback(struct k_timer *timer_id)
{
	/* Add item to system work queue so that it can be handled in task
	 * context because LEDs cannot be handed in interrupt context (mutex). */
	struct led *pLed = (struct led *)k_timer_user_data_get(timer_id);
	k_work_submit(&pLed->work);
}
