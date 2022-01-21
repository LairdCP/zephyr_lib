/**
 * @file lcz_power_direct.c
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
#include <hal/nrf_saadc.h>
#include <drivers/adc.h>
#include <logging/log_ctrl.h>

#ifdef CONFIG_REBOOT
#include <power/reboot.h>
#endif

#include "lcz_power.h"

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static struct adc_channel_cfg m_1st_channel_cfg = {
	.reference = ADC_REF_INTERNAL,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_CHANNEL_ID,
	.input_positive = NRF_SAADC_INPUT_VDD
};

static int16_t m_sample_buffer;
static struct k_mutex adc_mutex;
static struct k_timer power_timer;
static struct k_work power_work;
static bool timer_enabled;

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static void power_adc_to_voltage(int16_t adc, float scaling,
				 uint8_t *voltage_int, uint8_t *voltage_dec);
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
	/* Setup mutex work-queue and repetitive timer */
	k_mutex_init(&adc_mutex);
	k_timer_init(&power_timer, power_timer_callback, NULL);
	k_work_init(&power_work, system_workq_power_timer_handler);
}

void power_mode_set(bool enable)
{
	if (enable == true && timer_enabled == false) {
		k_timer_start(&power_timer, POWER_TIMER_PERIOD,
			      POWER_TIMER_PERIOD);
	} else if (enable == false && timer_enabled == true) {
		k_timer_stop(&power_timer);
	}
	timer_enabled = enable;

	if (enable == true) {
		/* Take a reading right away */
		power_run();
	}
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
static void power_adc_to_voltage(int16_t adc, float scaling,
				 uint8_t *voltage_int, uint8_t *voltage_dec)
{
	float voltage = (float)adc / ADC_LIMIT_VALUE * ADC_REFERENCE_VOLTAGE *
			scaling;

	*voltage_int = voltage;
	*voltage_dec = ((voltage - (float)(*voltage_int)) *
			ADC_DECIMAL_DIVISION_FACTOR);
}

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
	uint8_t voltage_int;
	uint8_t voltage_dec;

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
	k_mutex_lock(&adc_mutex, K_FOREVER);

	/* Measure voltage with 1/6 scaling */
	power_measure_adc(adc_dev, ADC_GAIN_1_6, sequence);
	power_adc_to_voltage(m_sample_buffer, ADC_GAIN_FACTOR_SIX, &voltage_int,
			     &voltage_dec);

	k_mutex_unlock(&adc_mutex);
	power_measurement_callback(voltage_int, voltage_dec);
}

static void system_workq_power_timer_handler(struct k_work *item)
{
	power_run();
}

/******************************************************************************/
/* Override in application                                                    */
/******************************************************************************/
__weak void power_measurement_callback(uint8_t integer, uint8_t decimal)
{
	ARG_UNUSED(integer);
	ARG_UNUSED(decimal);
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
