/**
 * @file lcz_sensor_event.c
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include "laird_utility_macros.h"
#include "lcz_sensor_event.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define PSCRS PREFIXED_SWITCH_CASE_RETURN_STRING

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
const char *lcz_sensor_event_get_reset_reason_string(uint8_t code)
{
	switch (code) {
		PSCRS(RESET_EVENT, POWER_UP);
		PSCRS(RESET_EVENT, RESETPIN);
		PSCRS(RESET_EVENT, DOG);
		PSCRS(RESET_EVENT, SREQ);
		PSCRS(RESET_EVENT, LOCKUP);
		PSCRS(RESET_EVENT, OFF);
		PSCRS(RESET_EVENT, LPCOMP);
		PSCRS(RESET_EVENT, DIF);
		PSCRS(RESET_EVENT, NFC);
		PSCRS(RESET_EVENT, VBUS);
	default:
		return "UNKNOWN";
	}
}

const char *lcz_sensor_event_get_string(uint8_t code)
{
	switch (code) {
		PSCRS(SENSOR_EVENT, RESERVED);
		PSCRS(SENSOR_EVENT, TEMPERATURE);
		PSCRS(SENSOR_EVENT, MAGNET);
		PSCRS(SENSOR_EVENT, MOVEMENT_START);
		PSCRS(SENSOR_EVENT, ALARM_HIGH_TEMP_1);
		PSCRS(SENSOR_EVENT, ALARM_HIGH_TEMP_2);
		PSCRS(SENSOR_EVENT, ALARM_HIGH_TEMP_CLEAR);
		PSCRS(SENSOR_EVENT, ALARM_LOW_TEMP_1);
		PSCRS(SENSOR_EVENT, ALARM_LOW_TEMP_2);
		PSCRS(SENSOR_EVENT, ALARM_LOW_TEMP_CLEAR);
		PSCRS(SENSOR_EVENT, ALARM_DELTA_TEMP);
		PSCRS(SENSOR_EVENT, ALARM_TEMPERATURE_RATE_OF_CHANGE);
		PSCRS(SENSOR_EVENT, BATTERY_GOOD);
		PSCRS(SENSOR_EVENT, ADV_ON_BUTTON);
		PSCRS(SENSOR_EVENT, NON_CONSECUTIVE);
		PSCRS(SENSOR_EVENT, IMPACT);
		PSCRS(SENSOR_EVENT, BATTERY_BAD);
		PSCRS(SENSOR_EVENT, RESET);
		PSCRS(SENSOR_EVENT, TEMPERATURE_1);
		PSCRS(SENSOR_EVENT, TEMPERATURE_2);
		PSCRS(SENSOR_EVENT, TEMPERATURE_3);
		PSCRS(SENSOR_EVENT, TEMPERATURE_4);
		PSCRS(SENSOR_EVENT, VOLTAGE_1);
		PSCRS(SENSOR_EVENT, VOLTAGE_2);
		PSCRS(SENSOR_EVENT, VOLTAGE_3);
		PSCRS(SENSOR_EVENT, VOLTAGE_4);
		PSCRS(SENSOR_EVENT, CURRENT_1);
		PSCRS(SENSOR_EVENT, CURRENT_2);
		PSCRS(SENSOR_EVENT, CURRENT_3);
		PSCRS(SENSOR_EVENT, CURRENT_4);
		PSCRS(SENSOR_EVENT, PRESSURE_1);
		PSCRS(SENSOR_EVENT, PRESSURE_2);
		PSCRS(SENSOR_EVENT, ULTRASONIC_1);
		PSCRS(SENSOR_EVENT, TEMPERATURE_ALARM);
		PSCRS(SENSOR_EVENT, ANALOG_ALARM);
		PSCRS(SENSOR_EVENT, DIGITAL_ALARM);
		PSCRS(SENSOR_EVENT, EPOCH);
		PSCRS(SENSOR_EVENT, RESET_V2);
		PSCRS(SENSOR_EVENT, TAMPER);
		PSCRS(SENSOR_EVENT, MOVEMENT_END);
	default:
		return "UNKNOWN";
	}
}