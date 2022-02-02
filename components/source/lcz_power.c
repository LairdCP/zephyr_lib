/**
 * @file lcz_power.c
 * @brief Voltage measurement control
 *
 * Copyright (c) 2020-2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_power, CONFIG_LCZ_POWER_LOG_LEVEL);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <stdio.h>
#include <zephyr/types.h>
#include <kernel.h>
#include <drivers/gpio.h>
#include <hal/nrf_saadc.h>
#include <drivers/adc.h>
#include <logging/log_ctrl.h>
#include <Framework.h>
#include <FrameworkMacros.h>
#include <framework_ids.h>
#include <framework_msgcodes.h>
#include <framework_types.h>
#include <BufferPool.h>
#include <locking_defs.h>
#include <locking.h>

#ifdef CONFIG_REBOOT
#include <power/reboot.h>
#endif

#include "lcz_power.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
/* clang-format off */
/* Port and pin number of the voltage measurement enabling functionality */
#if defined(CONFIG_BOARD_MG100)
#define MEASURE_ENABLE_PORT DT_PROP(DT_NODELABEL(gpio1), label)
#define MEASURE_ENABLE_PIN 10
#elif defined(CONFIG_BOARD_PINNACLE_100_DVK)
#define MEASURE_ENABLE_PORT DT_PROP(DT_NODELABEL(gpio0), label)
#define MEASURE_ENABLE_PIN 28
#else
#error "A measurement enable pin must be defined for this board."
#endif

/* clang-format off */
#define ADC_VOLTAGE_TOP_RESISTOR     14.1
#define ADC_VOLTAGE_BOTTOM_RESISTOR  1.1
#define MEASURE_STATUS_ENABLE        1
#define MEASURE_STATUS_DISABLE       0
/* clang-format on */

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
/* NEED CONFIG_ADC_CONFIGURABLE_INPUTS */

static struct adc_channel_cfg m_1st_channel_cfg = {
	.reference = ADC_REF_INTERNAL,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_CHANNEL_ID,
#if defined CONFIG_BOARD_PINNACLE_100_DVK
	.input_positive = NRF_SAADC_INPUT_AIN5
#elif defined CONFIG_BOARD_MG100
	.input_positive = NRF_SAADC_INPUT_AIN0
#else
#error "An ADC input must be defined for this hardware variant."
#endif
};

static int16_t m_sample_buffer;
static struct k_timer power_timer;
static struct k_work power_work;
static bool timer_enabled;
static uint32_t timer_interval = DEFAULT_POWER_TIMER_PERIOD_MS;

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static bool power_measure_adc(const struct device *adc_dev, enum adc_gain gain,
			      const struct adc_sequence sequence);
static void power_run(void);
static void system_workq_power_timer_handler(struct k_work *item);
static void power_timer_callback(struct k_timer *timer_id);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void power_init(void)
{
	int ret;

	/* Setup work-queue and repetitive timer */
	k_timer_init(&power_timer, power_timer_callback, NULL);
	k_work_init(&power_work, system_workq_power_timer_handler);

	/* Configure the VIN_ADC_EN pin as an output set low to disable the
	   power supply voltage measurement */
	ret = gpio_pin_configure(device_get_binding(MEASURE_ENABLE_PORT),
				 MEASURE_ENABLE_PIN, (GPIO_OUTPUT));
	if (ret) {
		LOG_ERR("Error configuring power GPIO");
		return;
	}

	ret = gpio_pin_set(device_get_binding(MEASURE_ENABLE_PORT),
			   MEASURE_ENABLE_PIN, MEASURE_STATUS_DISABLE);
	if (ret) {
		LOG_ERR("Error setting power GPIO");
		return;
	}
}

void power_mode_set(bool enable)
{
	if (enable == true && timer_enabled == false) {
		k_timer_start(&power_timer, K_MSEC(timer_interval),
			      K_MSEC(timer_interval));
	} else if (enable == false && timer_enabled == true) {
		k_timer_stop(&power_timer);
	}
	timer_enabled = enable;

	if (enable == true) {
		/* Take a reading right away */
		power_run();
	}
}

void power_interval_set(uint32_t interval_time)
{
	if (interval_time >= MINIMUM_POWER_TIMER_PERIOD_MS) {
		timer_interval = interval_time;
	}
}

uint32_t power_interval_get(void)
{
	return timer_interval;
}

#ifdef CONFIG_REBOOT
void power_reboot_module(uint8_t type)
{
	/* Log panic will cause all buffered logs to be output */
	LOG_INF("Rebooting module%s...",
		(type == REBOOT_TYPE_BOOTLOADER ? " into UART bootloader" :
						  ""));
#if CONFIG_LOG
	log_panic();
#endif

	/* And reboot the module */
	sys_reboot((type == REBOOT_TYPE_BOOTLOADER ? GPREGRET_BOOTLOADER_VALUE :
						     0));
}
#endif

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static bool power_measure_adc(const struct device *adc_dev, enum adc_gain gain,
			      const struct adc_sequence sequence)
{
	int ret = 0;

	/* Setup ADC with desired gain */
	m_1st_channel_cfg.gain = gain;
	ret = adc_channel_setup(adc_dev, &m_1st_channel_cfg);
	if (ret) {
		LOG_ERR("adc_channel_setup failed with %d", ret);
		return false;
	}

	/* Take ADC reading */
	ret = adc_read(adc_dev, &sequence);
	if (ret) {
		LOG_ERR("adc_read failed with %d", ret);
		return false;
	}

	return true;
}

static void power_run(void)
{
	int ret;
	bool finished = false;
	uint8_t scaling;

	/* Find the ADC device */
	const struct device *adc_dev = device_get_binding(ADC0);
	if (adc_dev == NULL) {
		LOG_ERR("ADC device name %s not found", ADC0);
		return;
	}

	(void)memset(&m_sample_buffer, 0, sizeof(m_sample_buffer));

	const struct adc_sequence sequence = {
		.channels = BIT(ADC_CHANNEL_ID),
		.buffer = &m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution = ADC_RESOLUTION,
	};

	/* Prevent other ADC uses */
	locking_take(LOCKING_ID_adc, K_FOREVER);

	/* Enable power supply voltage to be monitored */
	ret = gpio_pin_set(device_get_binding(MEASURE_ENABLE_PORT),
			   MEASURE_ENABLE_PIN, MEASURE_STATUS_ENABLE);
	if (ret) {
		LOG_ERR("Error setting power GPIO");
		return;
	}

	/* Measure voltage with 1/2 scaling which is suitable for higher
	   voltage supplies */
	power_measure_adc(adc_dev, ADC_GAIN_1_2, sequence);
	scaling = ADC_GAIN_FACTOR_TWO;

	if (m_sample_buffer >= ADC_SATURATION) {
		/* We have reached saturation point, do not try the next ADC
		   scaling */
		finished = true;
	}

	if (finished == false) {
		/* Measure voltage with unity scaling which is suitable for
		   medium voltage supplies */
		power_measure_adc(adc_dev, ADC_GAIN_1, sequence);
		scaling = ADC_GAIN_FACTOR_ONE;

		if (m_sample_buffer >= ADC_SATURATION) {
			/* We have reached saturation point, do not try the
			   next ADC scaling */
			finished = true;
		}
	}

	if (finished == false) {
		/* Measure voltage with double scaling which is suitable for
		   low voltage supplies, such as 2xAA batteries */
		power_measure_adc(adc_dev, ADC_GAIN_2, sequence);
		scaling = ADC_GAIN_FACTOR_HALF;
	}

	/* Disable the voltage monitoring FET */
	ret = gpio_pin_set(device_get_binding(MEASURE_ENABLE_PORT),
			   MEASURE_ENABLE_PIN, MEASURE_STATUS_DISABLE);

	if (ret) {
		LOG_ERR("Error setting power GPIO");
	}

	locking_give(LOCKING_ID_adc);

	power_measure_msg_t *fmsg = (power_measure_msg_t *)BufferPool_Take(
						sizeof(power_measure_msg_t));

	if (fmsg != NULL) {
		fmsg->header.msgCode = FMC_POWER_MEASURED;
		fmsg->header.txId = FWK_ID_POWER;
		fmsg->header.rxId = FWK_ID_RESERVED;
		fmsg->instance = 0;
		fmsg->configuration = LCZ_POWER_CONFIGURATION_POTENTIAL_DIVIDER;

		if (scaling == ADC_GAIN_FACTOR_TWO) {
			fmsg->gain = ADC_GAIN_2;
		} else if (scaling == ADC_GAIN_FACTOR_ONE) {
			fmsg->gain = ADC_GAIN_1;
		} else if (scaling == ADC_GAIN_FACTOR_HALF) {
			fmsg->gain = ADC_GAIN_1_2;
		}

		fmsg->voltage = (float)m_sample_buffer / ADC_LIMIT_VALUE *
				ADC_REFERENCE_VOLTAGE * scaling;

		Framework_Broadcast((FwkMsg_t *)fmsg,
				    sizeof(power_measure_msg_t));
	}
}

static void system_workq_power_timer_handler(struct k_work *item)
{
	power_run();
}

/******************************************************************************/
/* Interrupt Service Routines                                                 */
/******************************************************************************/
static void power_timer_callback(struct k_timer *timer_id)
{
	/* Add item to system work queue so that it can be handled in task
	 * context because ADC cannot be used in interrupt context (mutex). */
	k_work_submit(&power_work);
}
