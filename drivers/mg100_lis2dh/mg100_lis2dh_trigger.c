/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis2dh

#include <sys/util.h>
#include <kernel.h>
#include <logging/log.h>

#define START_TRIG_INT1 0
#define TRIGGED_INT1 4

LOG_MODULE_DECLARE(lis2dh, CONFIG_SENSOR_LOG_LEVEL);
#include "mg100_lis2dh.h"

static inline void setup_int1(const struct device *dev, bool enable)
{
	struct lis2dh_data *lis2dh = dev->data;

	gpio_pin_interrupt_configure(lis2dh->gpio_int1, LIS2DH_INT1_GPIOS_PIN,
				     enable ? GPIO_INT_EDGE_TO_ACTIVE :
					      GPIO_INT_DISABLE);
}

static int lis2dh_trigger_drdy_set(const struct device *dev,
				   enum sensor_channel chan,
				   sensor_trigger_handler_t handler)
{
	struct lis2dh_data *lis2dh = dev->data;
	int status;

	setup_int1(dev, false);

	/* cancel potentially pending trigger */
	atomic_clear_bit(&lis2dh->trig_flags, TRIGGED_INT1);

	status = lis2dh->hw_tf->update_reg(dev, LIS2DH_REG_CTRL3,
					   LIS2DH_EN_DRDY1_INT1, 0);

	lis2dh->handler_drdy = handler;
	if ((handler == NULL) || (status < 0)) {
		return status;
	}

	lis2dh->chan_drdy = chan;

	/* serialize start of int1 in thread to synchronize output sampling
	 * and first interrupt. this avoids concurrent bus context access.
	 */
	atomic_set_bit(&lis2dh->trig_flags, START_TRIG_INT1);
#if defined(CONFIG_MG100_LIS2DH_TRIGGER_OWN_THREAD)
	k_sem_give(&lis2dh->gpio_sem);
#elif defined(CONFIG_MG100_LIS2DH_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lis2dh->work);
#endif
	return 0;
}

#define LIS2DH_ANYM_CFG                                                        \
	(LIS2DH_INT_CFG_ZHIE_ZUPE | LIS2DH_INT_CFG_YHIE_YUPE |                 \
	 LIS2DH_INT_CFG_XHIE_XUPE)

static int lis2dh_trigger_anym_set(const struct device *dev,
				   sensor_trigger_handler_t handler)
{
	struct lis2dh_data *lis2dh = dev->data;
	int status;
	uint8_t reg_val;

	status = lis2dh_init_interrupt(dev);
	if (status < 0) {
		LOG_ERR("Failed to initialize interrupts.");
		return status;
	}

	setup_int1(dev, true);

	/* cancel potentially pending trigger */
	atomic_clear_bit(&lis2dh->trig_flags, TRIGGED_INT1);

	status = lis2dh->hw_tf->update_reg(dev, LIS2DH_REG_CTRL3,
					   LIS2DH_EN_DRDY1_INT1, 0);

	/* disable all interrupt 1 events */
	status = lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_INT1_CFG, 0);

	/* make sure any pending interrupt is cleared */
	status = lis2dh->hw_tf->read_reg(dev, LIS2DH_REG_INT1_SRC, &reg_val);

	lis2dh->handler_anymotion = handler;
	if ((handler == NULL) || (status < 0)) {
		return status;
	}

	/* serialize start of int1 in thread to synchronize output sampling
	 * and first interrupt. this avoids concurrent bus context access.
	 */
	atomic_set_bit(&lis2dh->trig_flags, START_TRIG_INT1);
#if defined(CONFIG_MG100_LIS2DH_TRIGGER_OWN_THREAD)
	k_sem_give(&lis2dh->gpio_sem);
#elif defined(CONFIG_MG100_LIS2DH_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lis2dh->work);
#endif
	return 0;
}

static int lis2dh_start_trigger_int1(const struct device *dev)
{
	struct lis2dh_data *lis2dh = dev->data;

	setup_int1(dev, true);

	return lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_INT1_CFG,
					LIS2DH_ANYM_CFG);
}

int lis2dh_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	if (trig->type == SENSOR_TRIG_DATA_READY &&
	    trig->chan == SENSOR_CHAN_ACCEL_XYZ) {
		return lis2dh_trigger_drdy_set(dev, trig->chan, handler);
	} else if (trig->type == SENSOR_TRIG_DELTA) {
		return lis2dh_trigger_anym_set(dev, handler);
	}

	return -ENOTSUP;
}

int lis2dh_acc_slope_config(const struct device *dev,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	struct lis2dh_data *lis2dh = dev->data;
	int status;

	if (attr == SENSOR_ATTR_SLOPE_TH) {
		uint8_t range_g, reg_val;
		uint32_t slope_th_ums2;

		status = lis2dh->hw_tf->read_reg(dev, LIS2DH_REG_CTRL4,
						 &reg_val);
		if (status < 0) {
			return status;
		}

		/* fs reg value is in the range 0 (2g) - 3 (16g) */
		range_g = 2 * (1 << ((LIS2DH_FS_MASK & reg_val) >>
				     LIS2DH_FS_SHIFT));

		slope_th_ums2 = val->val1 * 1000000 + val->val2;

		/* make sure the provided threshold does not exceed range */
		if ((slope_th_ums2 - 1) > (range_g * SENSOR_G)) {
			return -EINVAL;
		}

		/* 7 bit full range value */
		reg_val = 128 / range_g * (slope_th_ums2 - 1) / SENSOR_G;

		LOG_INF("int1_ths=0x%x range_g=%d ums2=%u", reg_val, range_g,
			slope_th_ums2 - 1);

		status = lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_INT1_THS,
						  reg_val);
	} else { /* SENSOR_ATTR_SLOPE_DUR */
		/*
		 * slope duration is measured in number of samples:
		 * N/ODR where N is the register value
		 */
		if (val->val1 < 0 || val->val1 > 127) {
			return -ENOTSUP;
		}

		LOG_INF("int1_dur=0x%x", val->val1);

		status = lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_INT1_DUR,
						  val->val1);
	}

	return status;
}

static void lis2dh_gpio_int1_callback(const struct device *dev,
				      struct gpio_callback *cb, uint32_t pins)
{
	struct lis2dh_data *lis2dh =
		CONTAINER_OF(cb, struct lis2dh_data, gpio_int1_cb);

	ARG_UNUSED(pins);

	atomic_set_bit(&lis2dh->trig_flags, TRIGGED_INT1);

#if defined(CONFIG_MG100_LIS2DH_TRIGGER_OWN_THREAD)
	k_sem_give(&lis2dh->gpio_sem);
#elif defined(CONFIG_MG100_LIS2DH_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lis2dh->work);
#endif
}

static void lis2dh_thread_cb(const struct device *dev)
{
	struct lis2dh_data *lis2dh = dev->data;
	int status;
	uint8_t reg_val;

	if (unlikely(atomic_test_and_clear_bit(&lis2dh->trig_flags,
					       START_TRIG_INT1))) {
		status = lis2dh_start_trigger_int1(dev);

		if (unlikely(status < 0)) {
			LOG_ERR("lis2dh_start_trigger_int1: %d", status);
		}
		return;
	}

	if (atomic_test_and_clear_bit(&lis2dh->trig_flags, TRIGGED_INT1)) {
		struct sensor_trigger anym_trigger = {
			.type = SENSOR_TRIG_DELTA,
			.chan = lis2dh->chan_drdy,
		};

		/* clear interrupt 1 to de-assert int1 line */
		status = lis2dh->hw_tf->read_reg(dev, LIS2DH_REG_INT1_SRC,
						 &reg_val);

		if (status < 0) {
			LOG_ERR("clearing interrupt 1 failed: %d", status);
			return;
		}

		if (likely(lis2dh->handler_anymotion != NULL)) {
			lis2dh->handler_anymotion(dev, &anym_trigger);
		}

		LOG_DBG("@tick=%u int1_src=0x%x", k_cycle_get_32(), reg_val);

		return;
	}
}

#ifdef CONFIG_MG100_LIS2DH_TRIGGER_OWN_THREAD
static void lis2dh_thread(struct lis2dh_data *lis2dh)
{
	while (1) {
		k_sem_take(&lis2dh->gpio_sem, K_FOREVER);
		lis2dh_thread_cb(lis2dh->dev);
	}
}
#endif

#ifdef CONFIG_MG100_LIS2DH_TRIGGER_GLOBAL_THREAD
static void lis2dh_work_cb(struct k_work *work)
{
	struct lis2dh_data *lis2dh =
		CONTAINER_OF(work, struct lis2dh_data, work);

	lis2dh_thread_cb(lis2dh->dev);
}
#endif

int lis2dh_init_interrupt(const struct device *dev)
{
	struct lis2dh_data *lis2dh = dev->data;
	int status;
	uint8_t raw[2];

	/* setup data ready gpio interrupt */
	lis2dh->gpio_int1 = device_get_binding(LIS2DH_INT1_GPIO_DEV_NAME);
	if (lis2dh->gpio_int1 == NULL) {
		LOG_ERR("Cannot get pointer to %s device",
			LIS2DH_INT1_GPIO_DEV_NAME);
		return -EINVAL;
	}

	lis2dh->dev = dev;
#if defined(CONFIG_MG100_LIS2DH_TRIGGER_OWN_THREAD)
	k_sem_init(&lis2dh->gpio_sem, 0, UINT_MAX);

	k_thread_create(&lis2dh->thread, lis2dh->thread_stack,
			CONFIG_MG100_LIS2DH_THREAD_STACK_SIZE,
			(k_thread_entry_t)lis2dh_thread, lis2dh, NULL, NULL,
			K_PRIO_COOP(CONFIG_MG100_LIS2DH_THREAD_PRIORITY), 0,
			K_NO_WAIT);
#elif defined(CONFIG_MG100_LIS2DH_TRIGGER_GLOBAL_THREAD)
	lis2dh->work.handler = lis2dh_work_cb;
#endif

	/* any motion int1 gpio configuration */
	status = gpio_pin_configure(lis2dh->gpio_int1, LIS2DH_INT1_GPIOS_PIN,
				    GPIO_INPUT | LIS2DH_INT1_GPIOS_FLAGS);
	if (status < 0) {
		LOG_ERR("Could not configure gpio %d", LIS2DH_INT1_GPIOS_PIN);
		return status;
	}

	gpio_init_callback(&lis2dh->gpio_int1_cb, lis2dh_gpio_int1_callback,
			   BIT(LIS2DH_INT1_GPIOS_PIN));

	/* callback is going to be enabled by trigger setting function */
	status = gpio_add_callback(lis2dh->gpio_int1, &lis2dh->gpio_int1_cb);
	if (status < 0) {
		LOG_ERR("Could not add gpio int1 callback (%d)", status);
		return status;
	}

	LOG_INF("int1 on pin=%d", LIS2DH_INT1_GPIOS_PIN);

	/* disable interrupt 1 in case of warm (re)boot */
	status = lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_INT1_CFG, 0);
	if (status < 0) {
		LOG_ERR("Interrupt 1 disable reg write failed (%d)", status);
		return status;
	}

	(void)memset(raw, 0, sizeof(raw));
	status = lis2dh->hw_tf->write_data(dev, LIS2DH_REG_INT1_THS, raw,
					   sizeof(raw));
	if (status < 0) {
		LOG_ERR("Burst write to INT1 THS failed (%d)", status);
		return status;
	}

	/* enable interrupt 1 on int1 line */
	status = lis2dh->hw_tf->update_reg(dev, LIS2DH_REG_CTRL3,
					   LIS2DH_EN_INT1_INT1,
					   LIS2DH_EN_INT1_INT1);

	/* latch int1 line interrupt */
	status = lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_CTRL5,
					  LIS2DH_EN_LIR_INT1);
	if (status < 0) {
		LOG_ERR("INT1 latch enable reg write failed (%d)", status);
		return status;
	}

	return status;
}
