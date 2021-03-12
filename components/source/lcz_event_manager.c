/*
 * @file lcz_event_manager.c
 * @brief Public interface to the Event manager.
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include <zephyr.h>
#include "lcz_sensor_event.h"
#include "lcz_event_manager.h"
#include "lcz_event_manager_file_handler.h"
#include "lcz_qrtc.h"

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void lcz_event_manager_initialise(void)
{
	/* Create kernel objects */
	lcz_event_manager_file_handler_initialise();
}

uint32_t lcz_event_manager_add_sensor_event(SensorEventType_t sensor_event_type,
					SensorEventData_t *sensor_event_data)
{
	uint32_t time_stamp = 0;

	/* Check if the QRTC has been set */
	if (lcz_qrtc_epoch_was_set()) {
		/* Yes, so we can save this event */
		/* Get the current time */
		time_stamp = lcz_qrtc_get_epoch();
		/* Now update the record accordingly */
		lcz_event_manager_file_handler_add_event(
			sensor_event_type, sensor_event_data, time_stamp);
	}
	return(time_stamp);
}

int lcz_event_manager_prepare_log_file(uint8_t *log_path)
{
	return (lcz_event_manager_file_handler_build_file(log_path));
}

int lcz_event_manager_delete_log_file()
{
	return (lcz_event_manager_file_handler_delete_file());
}

SensorEvent_t *lcz_event_manager_get_next_event(uint32_t start_time_stamp,
						uint16_t *count, uint16_t index)
{
	SensorEvent_t *sensor_event = NULL;

	sensor_event = lcz_event_manager_file_handler_get_indexed_event_at_timestamp(
		start_time_stamp, index, count);
	/* Exit with the event if found */
	return (sensor_event);
}
