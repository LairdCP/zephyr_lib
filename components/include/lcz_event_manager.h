/*
 * @file lcz_event_manager.h
 * @brief Public interface to the Event manager.
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LCZ_EVENT_MANAGER_H

#define LCZ_EVENT_MANAGER_H

/* This is the length of paths generated for event manager files */
#define LCZ_EVENT_MANAGER_FILENAME_SIZE 40

/** @brief Initialises the Event Manager.
 *
 */
void lcz_event_manager_initialise(void);

/** @brief Adds a sensor event to the sensor event queue
 *
 * @param [in]sensor_event_type - The event type.
 * @param [in]sensor_event_data - The event data.
 * @return The timestamp recorded for event.
 */
uint32_t
lcz_event_manager_add_sensor_event(SensorEventType_t sensor_event_type,
				   SensorEventData_t *sensor_event_data);

/** @brief Prepares an event log for external use
 *
 * @param [out]log_path - The absolute path of the log file.
 * @param [out]log_file_size - The file size in bytes.
 * @return Zero for success, a non-zero error code otherwise.
 */
int lcz_event_manager_prepare_log_file(uint8_t *log_path,
				       uint32_t *log_file_size);

/** @brief Deletes the last created log file
 *  @return Zero for success, a non-zero error code otherwise.
 */
int lcz_event_manager_delete_log_file(void);

/** @brief Reads a log from the event log for internal system use.
 *
 * @param [in]start_time_stamp - The timestamp where to find the next event.
 * @param [out]count - The number of events at this timestamp.
 * @param [out]index - The index of the event to read.
 * @return The requested event if found, NULL otherwise.
 */
SensorEvent_t *lcz_event_manager_get_next_event(uint32_t start_time_stamp,
						uint16_t *count,
						uint16_t index);

/** @brief Gets the status of the last create log file request.
 *         Note the enum type is cast as a U32 here to align with the type
 *         supported by the device API and to avoid pre-including the
 *         event manager file handler file before this one.
 *
 * @return The log file status.
 */
uint32_t lcz_event_manager_get_log_file_status(void);
#endif /* ifdef LCZ_EVENT_MANAGER_H */
