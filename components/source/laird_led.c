/**
 * @file laird_led.c
 * @brief LED control
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(laird_led);

#define LED_LOG_ERR(...) LOG_ERR(__VA_ARGS__)

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <drivers/gpio.h>
#include <kernel.h>

#include "laird_led.h"

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
	struct device *device_handle;
	uint32_t pin;
	bool on_when_high;
	bool pattern_busy;
	struct led_blink_pattern pattern;
	struct k_timer timer;
	struct k_work work;
	void (*pattern_complete_function)(void);
};

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static struct k_mutex led_mutex;
static struct led led[CONFIG_NUMBER_OF_LEDS];

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static void led_timer_callback(struct k_timer *timer_id);
static void system_workq_led_timer_handler(struct k_work *item);
static void turn_on(struct led *pLed);
static void turn_off(struct led *pLed);
static void change_state(struct led *pLed, bool state, bool blink);
static void led_bind_and_configure(struct led_configuration *pConfig);
static void led_bind_device(led_index_t index, const char *name);
static void led_configure_pin(led_index_t index, uint32_t pin);

static bool valid_index(led_index_t index);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void led_init(struct led_configuration *pConfig, size_t size)
{
	k_mutex_init(&led_mutex);
	TAKE_MUTEX(led_mutex);
	size_t i;
	for (i = 0; i < MIN(size, CONFIG_NUMBER_OF_LEDS); i++) {
		k_timer_init(&led[i].timer, led_timer_callback, NULL);
		k_timer_user_data_set(&led[i].timer, &led[i]);
		k_work_init(&led[i].work, system_workq_led_timer_handler);
		led_bind_and_configure(pConfig + i);
	}
	GIVE_MUTEX(led_mutex);
}

void led_turn_on(led_index_t index)
{
	if (!valid_index(index)) {
		return;
	}
	TAKE_MUTEX(led_mutex);
	change_state(&led[index], ON, DONT_BLINK);
	GIVE_MUTEX(led_mutex);
}

void led_turn_off(led_index_t index)
{
	if (!valid_index(index)) {
		return;
	}
	TAKE_MUTEX(led_mutex);
	change_state(&led[index], OFF, DONT_BLINK);
	GIVE_MUTEX(led_mutex);
}

void led_blink(led_index_t index, struct led_blink_pattern const *pPattern)
{
	if (!valid_index(index)) {
		return;
	}
	if (pPattern != NULL) {
		TAKE_MUTEX(led_mutex);
		led[index].pattern_busy = true;
		memcpy(&led[index].pattern, pPattern,
		       sizeof(struct led_blink_pattern));
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

void led_register_pattern_complete_function(led_index_t index,
					    void (*function)(void))
{
	if (!valid_index(index)) {
		return;
	}
	TAKE_MUTEX(led_mutex);
	led[index].pattern_complete_function = function;
	GIVE_MUTEX(led_mutex);
}

bool led_pattern_busy(led_index_t index)
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
static void led_bind_and_configure(struct led_configuration *pConfig)
{
	if (pConfig->index >= CONFIG_NUMBER_OF_LEDS) {
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
		LED_LOG_ERR("Cannot find %s!", name);
	}
}

static void led_configure_pin(led_index_t index, uint32_t pin)
{
	int ret;
	led[index].pin = pin;
	ret = gpio_pin_configure(led[index].device_handle, led[index].pin,
				 (GPIO_OUTPUT));
	if (ret) {
		LED_LOG_ERR("Error configuring GPIO");
	}
	ret = gpio_pin_set(led[index].device_handle, led[index].pin,
			   led[index].on_when_high ? 0 : 1);
	if (ret) {
		LED_LOG_ERR("Error setting GPIO state");
	}
}

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
	gpio_pin_set(pLed->device_handle, pLed->pin,
		     pLed->on_when_high ? 1 : 0);
}

static void turn_off(struct led *pLed)
{
	gpio_pin_set(pLed->device_handle, pLed->pin,
		     pLed->on_when_high ? 0 : 1);
}

static bool valid_index(led_index_t index)
{
	if (index < CONFIG_NUMBER_OF_LEDS) {
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
