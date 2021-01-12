/**
 * @file lcz_sensor_event.h
 * @brief Sensor event types for Laird Connectivity Bluetooth sensors.
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_SENSOR_EVENT_H__
#define __LCZ_SENSOR_EVENT_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Constants, Macros and Type Definitions                              */
/******************************************************************************/
typedef enum MAGNET_STATE { MAGNET_NEAR = 0, MAGNET_FAR } MagnetState_t;

typedef enum SENSOR_EVENT {
	SENSOR_EVENT_RESERVED = 0,
	SENSOR_EVENT_TEMPERATURE = 1,
	SENSOR_EVENT_MAGNET = 2, /* or proximity */
	SENSOR_EVENT_MOVEMENT = 3,
	SENSOR_EVENT_ALARM_HIGH_TEMP_1 = 4,
	SENSOR_EVENT_ALARM_HIGH_TEMP_2 = 5,
	SENSOR_EVENT_ALARM_HIGH_TEMP_CLEAR = 6,
	SENSOR_EVENT_ALARM_LOW_TEMP_1 = 7,
	SENSOR_EVENT_ALARM_LOW_TEMP_2 = 8,
	SENSOR_EVENT_ALARM_LOW_TEMP_CLEAR = 9,
	SENSOR_EVENT_ALARM_DELTA_TEMP = 10,
	SENSOR_EVENT_ALARM_TEMPERATURE_RATE_OF_CHANGE = 11,
	SENSOR_EVENT_BATTERY_GOOD = 12,
	SENSOR_EVENT_ADV_ON_BUTTON = 13,
	SENSOR_EVENT_RESERVED_14 = 14,
	SENSOR_EVENT_IMPACT = 15,
	SENSOR_EVENT_BATTERY_BAD = 16,
	SENSOR_EVENT_RESET = 17,
	/* BT6x */
	SENSOR_EVENT_TEMPERATURE_1 = 18,
	SENSOR_EVENT_TEMPERATURE_2 = 19,
	SENSOR_EVENT_TEMPERATURE_3 = 20,
	SENSOR_EVENT_TEMPERATURE_4 = 21,
	SENSOR_EVENT_VOLTAGE_1 = 22,
	SENSOR_EVENT_VOLTAGE_2 = 23,
	SENSOR_EVENT_VOLTAGE_3 = 24,
	SENSOR_EVENT_VOLTAGE_4 = 25,
	SENSOR_EVENT_CURRENT_1 = 26,
	SENSOR_EVENT_CURRENT_2 = 27,
	SENSOR_EVENT_CURRENT_3 = 28,
	SENSOR_EVENT_CURRENT_4 = 29,
	SENSOR_EVENT_PRESSURE_1 = 30,
	SENSOR_EVENT_PRESSURE_2 = 31,
	SENSOR_EVENT_ULTRASONIC_1 = 32,
	SENSOR_EVENT_TEMPERATURE_ALARM = 33,
	SENSOR_EVENT_ANALOG_ALARM = 34,
	SENSOR_EVENT_DIGITAL_ALARM = 35,

	NUMBER_OF_SENSOR_EVENTS
} SensorEventType_t;
BUILD_ASSERT(sizeof(SensorEventType_t) <= sizeof(uint8_t),
	     "Sensor Event enum too large");

/* The IG60 publishes events with these names and a voltage or temperature. */
#define IG60_GENERATED_EVENT_STR_BATTERY_GOOD "batteryGood"
#define IG60_GENERATED_EVENT_STR_BATTERY_BAD "batteryBad"
#define IG60_GENERATED_EVENT_STR_ALARM_HIGH_TEMP_1 "alarmHighTemp1"
#define IG60_GENERATED_EVENT_STR_ALARM_HIGH_TEMP_2 "alarmHighTemp2"
#define IG60_GENERATED_EVENT_STR_ALARM_HIGH_TEMP_CLEAR "alarmHighTempClear"
#define IG60_GENERATED_EVENT_STR_ALARM_LOW_TEMP_1 "alarmLowTemp1"
#define IG60_GENERATED_EVENT_STR_ALARM_LOW_TEMP_2 "alarmLowTemp2"
#define IG60_GENERATED_EVENT_STR_ALARM_LOW_TEMP_CLEAR "alarmLowTempClear"
#define IG60_GENERATED_EVENT_STR_ALARM_DELTA_TEMP "alarmDeltaTemp"
#define IG60_GENERATED_EVENT_STR_ADVERTISE_ON_BUTTON "advertiseOnButton"

/* This is the format in the sensor event log for the BT510.
 *
 * Salt is used internally by BT510 Sensor Log to differentiate
 * events with the same timestamp.
 */
struct SensorEventBt510 {
	uint32_t timestamp;
	union {
		uint16_t u16;
		uint8_t magnetState;
		int16_t temperatureCc;
		uint16_t batteryMv;
	} data;
	SensorEventType_t type;
	uint8_t salt;
} __packed;
typedef struct SensorEventBt510 SensorEventBt510_t;
BUILD_ASSERT(sizeof(SensorEventBt510_t) == 8, "Sensor event NOT packed");

struct SensorEvent {
	uint32_t timestamp;
	union {
		float f;
		uint32_t u32;
		int32_t s32;
	} data;
	SensorEventType_t type;
	uint8_t salt;
	uint8_t reserved1;
	uint8_t reserved2;
} __packed;
typedef struct SensorEvent SensorEvent_t;
BUILD_ASSERT(sizeof(SensorEvent_t) == 12, "Sensor event NOT packed");

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_SENSOR_EVENT_H__ */
