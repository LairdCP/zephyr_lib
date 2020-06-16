/*
 * Copyright (c) 2020 Laird Connectivity
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <logging/log.h>
#include "version.h"
#include "accelerometer.h"

#define SLOPE_THRESHOLD 12  // m/s^2
#define SLOPE_DURATION 6
void main(void)
{
	printk("Laird Connectivity %s\n", CONFIG_BOARD);
	config_accelerometer(LIS2DH_ACCEL_RANGE_8G, LIS2DH_ODR_5_100Hz, SLOPE_THRESHOLD, SLOPE_DURATION);
	/* The shell runs a loop so don't loop anything here, do it in a thread */
}
