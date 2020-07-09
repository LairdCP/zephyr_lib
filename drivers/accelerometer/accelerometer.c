/*
 * Copyright (c) 2020 Laird Connectivity
 */

#include <stdio.h>
#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include "accelerometer.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(Accel, LOG_LEVEL);
#define USE_PRINTF_FOR_ACCEL true
#define THREE_AXIS 3
static struct device *Sensor;
static struct sensor_trigger Trig;

static void fetch_and_display(struct device *sensor)
{
	static unsigned int count;
	struct sensor_value accel[THREE_AXIS];
	const char *overrun = "";
	int rc = sensor_sample_fetch(sensor);
	++count;
	if (rc == -EBADMSG) {
		/* Sample overrun.  Ignore in polled mode. */
		if (IS_ENABLED(CONFIG_LIS2DH_TRIGGER)) {
			overrun = "[OVERRUN] ";
		}
		rc = 0;
	}
	if (rc == 0) {
		rc = sensor_channel_get(sensor, Trig.chan, accel);
	}
	if (rc < 0) {
		LOG_ERR("ERROR: Update failed: %d\n", rc);
	} else {
#if USE_PRINTF_FOR_ACCEL == true
		printf("#%u%s: (%.1f, %.1f, %.1f)\n", count, overrun,
		       sensor_value_to_double(&accel[0]),
		       sensor_value_to_double(&accel[1]),
		       sensor_value_to_double(&accel[2]));
#else
		int accel_debug[THREE_AXIS];
		for (int i = 0; i < THREE_AXIS; i++) {
			accel_debug[i] =
				sensor_value_to_double(&accel[i]) * 100;
		}
		LOG_INF("#%u%s: (%d, %d %d)\n", count, overrun, accel_debug[0],
			accel_debug[1], accel_debug[2]);
#endif
	}
}

static void trigger_handler(struct device *dev, struct sensor_trigger *trig)
{
	fetch_and_display(dev);
}

static void initialize_Sensor(void)
{
	Sensor = device_get_binding(DT_LABEL(DT_INST(0, st_lis2dh)));

	if (Sensor == NULL) {
		LOG_ERR("Could not get %s device\n",
			DT_LABEL(DT_INST(0, st_lis2dh)));
		return;
	}
	Trig.type = SENSOR_TRIG_DELTA;
	Trig.chan = SENSOR_CHAN_ACCEL_XYZ;
}

/* Accleration in m/S^2: 9.80665 m/s^2 = 1G */
static void set_full_scale(uint16_t full_scale)
{
	int rc;
	struct sensor_value attr;
	attr.val1 = full_scale;
	attr.val2 = 0;
	rc = sensor_attr_set(Sensor, Trig.chan, SENSOR_ATTR_FULL_SCALE, &attr);
	if (rc != 0) {
		LOG_ERR("Failed to set Full Scale: %d\n", rc);
		return;
	}
	LOG_DBG("Full Scale: ms2 = %d, G = %d\n", attr.val1,
		sensor_ms2_to_g(&attr));
}

/* Values in Hz */
static void set_sample_frequency(uint16_t odr_hz)
{
	int rc;
	struct sensor_value odr;
	odr.val1 = odr_hz;
	odr.val2 = 0;
	rc = sensor_attr_set(Sensor, Trig.chan, SENSOR_ATTR_SAMPLING_FREQUENCY,
			     &odr);
	if (rc != 0) {
		LOG_ERR("Failed to set odr: %d\n", rc);
		return;
	}
	LOG_DBG("sampling freq = %d", odr.val1);
}

/* slope threshold Value is m/S^2 */
static void set_slope_threshold(uint16_t slope_threshold)
{
	int rc;
	struct sensor_value attr;
	attr.val1 = slope_threshold;
	attr.val2 = 0;
	rc = sensor_attr_set(Sensor, Trig.chan, SENSOR_ATTR_SLOPE_TH, &attr);
	if (rc != 0) {
		LOG_ERR("Failed to set threshold: %d\n", rc);
		return;
	}
	LOG_DBG("slope threshold = %d m/S^2", attr.val1);
}

/* number of slopes */
static void set_slope_duration(uint16_t slope_duration)
{
	int rc;
	struct sensor_value attr;
	attr.val1 = slope_duration;
	attr.val2 = 0;
	rc = sensor_attr_set(Sensor, Trig.chan, SENSOR_ATTR_SLOPE_DUR, &attr);
	if (rc != 0) {
		LOG_ERR("Failed to set slope duration: %d\n", rc);
		return;
	}
	LOG_DBG("slope duration = %d", attr.val1);
}

static void set_trigger(void)
{
	int rc;
	rc = sensor_trigger_set(Sensor, &Trig, trigger_handler);
	if (rc != 0) {
		LOG_ERR("Failed to set trigger: %d\n", rc);
		return;
	}
}

void config_accelerometer(uint16_t full_scale, uint16_t odr_hz,
			  uint16_t slope_threshold, uint16_t slope_duration)
{
	initialize_Sensor();
	set_full_scale(full_scale);
	set_sample_frequency(odr_hz);
	set_slope_threshold(slope_threshold);
	set_slope_duration(slope_duration);
	set_trigger();
}
