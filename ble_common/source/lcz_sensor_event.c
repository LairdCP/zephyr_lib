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
/* Global Function Definitions                                                */
/******************************************************************************/
const char *lcz_sensor_event_get_reset_reason_string(uint8_t code)
{
	switch (code) {
		PREFIXED_SWITCH_CASE_RETURN_STRING(RESET_EVENT, POWER_UP);
		PREFIXED_SWITCH_CASE_RETURN_STRING(RESET_EVENT, RESETPIN);
		PREFIXED_SWITCH_CASE_RETURN_STRING(RESET_EVENT, DOG);
		PREFIXED_SWITCH_CASE_RETURN_STRING(RESET_EVENT, SREQ);
		PREFIXED_SWITCH_CASE_RETURN_STRING(RESET_EVENT, LOCKUP);
		PREFIXED_SWITCH_CASE_RETURN_STRING(RESET_EVENT, OFF);
		PREFIXED_SWITCH_CASE_RETURN_STRING(RESET_EVENT, LPCOMP);
		PREFIXED_SWITCH_CASE_RETURN_STRING(RESET_EVENT, DIF);
		PREFIXED_SWITCH_CASE_RETURN_STRING(RESET_EVENT, NFC);
		PREFIXED_SWITCH_CASE_RETURN_STRING(RESET_EVENT, VBUS);
	default:
		return "UNKNOWN";
	}
}
