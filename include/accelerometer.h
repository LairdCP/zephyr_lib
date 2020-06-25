/*
 * Copyright (c) 2020 Laird Connectivity
 */

#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Accleration in m/S^2: 9.80665 m/s^2 = 1G */
#define LIS2DH_ACCEL_RANGE_2G 19
#define LIS2DH_ACCEL_RANGE_4G 39
#define LIS2DH_ACCEL_RANGE_8G 78
#define LIS2DH_ACCEL_RANGE_16G 156

/* Values in Hz */
#define LIS2DH_ODR_1_1Hz 1
#define LIS2DH_ODR_2_10Hz 10
#define LIS2DH_ODR_3_25Hz 25
#define LIS2DH_ODR_4_50Hz 50
#define LIS2DH_ODR_5_100Hz 100
#define LIS2DH_ODR_6_200Hz 200
#define LIS2DH_ODR_7_400Hz 400
#define LIS2DH_ODR_8_1_6kHz 1600
#define LIS2DH_ODR_9_NORMAL_1_25kHz 1250
#define LIS2DH_ODR_9_LOW_5kHz 5000

void config_accelerometer(uint16_t full_scale, uint16_t odr_hz,
			  uint16_t slope_threshold, uint16_t slope_duration);

#ifdef __cplusplus
}
#endif

#endif /* ACCELEROMETER_H */
