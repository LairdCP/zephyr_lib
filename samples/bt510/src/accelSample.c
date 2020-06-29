/*
 * Copyright (c) 2020 Laird Connectivity
 */

#include <zephyr.h>
#include <logging/log.h>
#include "accelerometer.h"
#include "accelSample.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(accelSample, LOG_LEVEL);
#define SLOPE_THRESHOLD 12 // m/s^2
#define SLOPE_DURATION 6

void sample_accel(void)
{
	config_accelerometer(LIS2DH_ACCEL_RANGE_8G, LIS2DH_ODR_5_100Hz,
			     SLOPE_THRESHOLD, SLOPE_DURATION);
}
