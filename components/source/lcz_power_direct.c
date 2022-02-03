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
#include <nrfx_power.h>
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

#if defined(CONFIG_LCZ_POWER_POWER_FAILURE) && defined(CONFIG_MPSL)
#include <mpsl.h>
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
static struct k_timer power_timer;
static struct k_work power_work;
static struct k_work power_fail_work;
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

#ifdef CONFIG_LCZ_POWER_POWER_FAILURE
static void system_workq_power_fail_handler(struct k_work *item);
#endif

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void power_init(void)
{
	/* Setup work-queue and repetitive timer */
	k_timer_init(&power_timer, power_timer_callback, NULL);
	k_work_init(&power_work, system_workq_power_timer_handler);

#ifdef CONFIG_LCZ_POWER_POWER_FAILURE
	k_work_init(&power_fail_work, system_workq_power_fail_handler);
#endif
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

#ifdef CONFIG_LCZ_POWER_POWER_FAILURE
void power_fail_set(bool enable, nrf_power_pof_thr_t power_level)
{
	if (enable == true) {
		nrf_power_pofcon_set(NRF_POWER, true, power_level);
		nrf_power_int_enable(NRF_POWER, NRF_POWER_INT_POFWARN_MASK);
	} else {
		if (nrf_power_int_enable_check(NRF_POWER,
					       NRF_POWER_INT_POFWARN_MASK)) {
			nrf_power_int_disable(NRF_POWER,
					      NRF_POWER_INT_POFWARN_MASK);
		}
		nrf_power_pofcon_set(NRF_POWER, false, NRF_POWER_POFTHR_V28);
	}
}
#endif

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

	/* Measure voltage with 1/6 scaling */
	power_measure_adc(adc_dev, ADC_GAIN_1_6, sequence);

	locking_give(LOCKING_ID_adc);

	power_measure_msg_t *fmsg = (power_measure_msg_t *)BufferPool_Take(
						sizeof(power_measure_msg_t));

	if (fmsg != NULL) {
		fmsg->header.msgCode = FMC_POWER_MEASURED;
		fmsg->header.txId = FWK_ID_POWER;
		fmsg->header.rxId = FWK_ID_RESERVED;
		fmsg->instance = 0;
		fmsg->configuration = LCZ_POWER_CONFIGURATION_DIRECT;
		fmsg->gain = ADC_GAIN_1_6;
		fmsg->voltage = (float)m_sample_buffer / ADC_LIMIT_VALUE *
				ADC_REFERENCE_VOLTAGE * ADC_GAIN_FACTOR_SIX;

		Framework_Broadcast((FwkMsg_t *)fmsg,
				    sizeof(power_measure_msg_t));
	}
}

static void system_workq_power_timer_handler(struct k_work *item)
{
	power_run();
}

#ifdef CONFIG_LCZ_POWER_POWER_FAILURE
static void system_workq_power_fail_handler(struct k_work *item)
{
	FRAMEWORK_MSG_CREATE_AND_BROADCAST(FWK_ID_POWER,
					   FMC_POWER_FAIL_TRIGGERED);
}
#endif

/******************************************************************************/
/* Interrupt Service Routines                                                 */
/******************************************************************************/
static void power_timer_callback(struct k_timer *timer_id)
{
	/* Add item to system work queue so that it can be handled in task
	 * context because ADC cannot be used in interrupt context (mutex)
	 */
	k_work_submit(&power_work);
}

#ifdef CONFIG_LCZ_POWER_POWER_FAILURE
void nrfx_power_clock_irq_handler(void)
{
#ifdef CONFIG_MPSL
        MPSL_IRQ_CLOCK_Handler();
#endif

	if (NRF_POWER->EVENTS_POFWARN == 1) {
		/* Power failure warning, clear event and queue up a work item
		 * to send a message
		 */
		NRF_POWER->EVENTS_POFWARN = 0;
		k_work_submit(&power_fail_work);
	}
}
#endif
