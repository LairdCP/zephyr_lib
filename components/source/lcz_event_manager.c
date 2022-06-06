/*
 * @file lcz_event_manager.c
 * @brief Public interface to the Event manager.
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <zephyr.h>
#include "lcz_sensor_event.h"
#include "lcz_event_manager.h"
#include "lcz_event_manager_file_handler.h"
#include "lcz_qrtc.h"

/***************************************************************************************************/
/* Global Function Definitions                                                                     */
/***************************************************************************************************/
void lcz_event_manager_initialise(bool save_to_flash)
{
	/* Create kernel objects */
	lcz_event_manager_file_handler_initialise(save_to_flash);
}

uint32_t lcz_event_manager_add_sensor_event(SensorEventType_t sensor_event_type,
					    SensorEventData_t *sensor_event_data)
{
	uint32_t time_stamp = 0;

	/* Check if the QRTC has been set */
	if (lcz_qrtc_epoch_was_set()) {
		/* Yes, so we can save this event, get the current time */
		time_stamp = lcz_qrtc_get_epoch();
		/* Now update the record accordingly */
		lcz_event_manager_file_handler_add_event(sensor_event_type, sensor_event_data,
							 time_stamp);
	}
	return (time_stamp);
}

int lcz_event_manager_prepare_log_file(uint8_t *log_path, uint32_t *log_file_size)
{
	int result = -EBUSY;

	/* Assume log file creation will fail */
	*log_file_size = 0;

	/* Don't allow a new file to be created if we're already preparing one */
	if (lcz_event_manager_file_handler_get_log_file_status() != LOG_FILE_STATUS_PREPARING) {
		result = lcz_event_manager_file_handler_build_file(log_path, log_file_size, true);
	}
	return (result);
}

int lcz_event_manager_delete_log_file(void)
{
	return (lcz_event_manager_file_handler_delete_file());
}

SensorEvent_t *lcz_event_manager_get_next_event(uint32_t start_time_stamp, uint16_t *count,
						uint16_t index)
{
	SensorEvent_t *sensor_event = NULL;

	sensor_event = lcz_event_manager_file_handler_get_indexed_event_at_timestamp(
		start_time_stamp, index, count);
	/* Exit with the event if found */
	return (sensor_event);
}

uint32_t lcz_event_manager_get_log_file_status(void)
{
	return (((uint32_t)(lcz_event_manager_file_handler_get_log_file_status())));
}

int lcz_event_manager_prepare_test_log_file(DummyLogFileProperties_t *dummy_log_file_properties,
					    uint8_t *log_path, uint32_t *log_file_size)
{
	int result = -EBUSY;

	/* Assume log file creation will fail */
	*log_file_size = 0;

	/* Check a file is not being built before starting a new one */
	if (lcz_event_manager_file_handler_get_log_file_status() != LOG_FILE_STATUS_PREPARING) {
		result = lcz_event_manager_file_handler_build_test_file(
			dummy_log_file_properties, log_path, log_file_size, true);
	}
	return (result);
}

void lcz_event_manager_set_logging_state(bool save_to_flash)
{
	lcz_event_manager_file_handler_set_logging_state(save_to_flash);
}

void lcz_event_manager_factory_reset(void)
{
	lcz_event_manager_file_handler_factory_reset();
}
