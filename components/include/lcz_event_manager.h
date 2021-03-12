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
 * @param [in]sensorEventType - The event type.
 * @param [in]pSensorEventData - The event data.
 * @return The timestamp recorded for event.
 */
uint32_t lcz_event_manager_add_sensor_event(SensorEventType_t sensorEventType,
					SensorEventData_t *pSensorEventData);

/** @brief Prepares an event log for external use
 *
 * @param [out]logPath - The absolute path of the log file.
 * @return Zero for success, a non-zero error code otherwise.
 */
int lcz_event_manager_prepare_log_file(uint8_t *logPath);

/** @brief Deletes the last created log file
 *  @return Zero for success, a non-zero error code otherwise.
 */
int lcz_event_manager_delete_log_file();

/** @brief Reads a log from the event log for internal system use.
 *
 * @param [in]startTimestamp - The timestamp where to find the next event.
 * @param [out]count - The number of events at this timestamp.
 * @param [out]index - The index of the event to read.
 * @return The requested event if found, NULL otherwise.
 */
SensorEvent_t *lcz_event_manager_get_next_event(uint32_t startTimestamp,
						uint16_t *count,
						uint16_t index);

#endif /* ifdef LCZ_EVENT_MANAGER_H */
