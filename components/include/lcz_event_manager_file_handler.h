/*
 * @file lcz_event_manager_file_handler.h
 * @brief Interface to the Event manager data handling functions.
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LCZ_EVENT_MANAGER_FILE_HANDLER_H

#define LCZ_EVENT_MANAGER_FILE_HANDLER_H

/******************************************************************************/
/* Global Constants, Macros and Type Definitions                              */
/******************************************************************************/
/* This is used to indicate the status of the last log file creation request. */
typedef enum {
	LOG_FILE_STATUS_WAITING = 0,
	LOG_FILE_STATUS_PREPARING,
	LOG_FILE_STATUS_READY,
	LOG_FILE_STATUS_FAILED,
	LOG_FILE_STATUS_COUNT
} LogFileStatus_t;

/* These are the types that can be used for data in dummy log files. */
typedef enum {
	DUMMY_LOG_DATA_TYPE_BOOL = 0,
	DUMMY_LOG_DATA_TYPE_U32,
	DUMMY_LOG_DATA_TYPE_FLOAT,
	DUMMY_LOG_DATA_TYPE_COUNT
} DummyLogDataType_t;

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/** @brief Initialises kernel objects
 *
 *  @param [in]save_to_flash - Sets the initial state of the save to flash flag
 */
void lcz_event_manager_file_handler_initialise(bool save_to_flash);

/** @brief Adds an event to the event log.
 *
 *  @param [in]sensorEventType - The sensor event type.
 *  @param [in]pSensorEventData - The data associated with the event.
 *  @param [in]timestamp - The timestamp associated with the event.
 */
void lcz_event_manager_file_handler_add_event(
	SensorEventType_t sensorEventType, SensorEventData_t *pSensorEventData,
	uint32_t timestamp);

/** @brief Builds an event log file for reading over the device user
 *         interfaces.
 *
 *  @param [out]absFilePath - The absolute file path where the file was created
 *  @param [out]file_size - The size of the file in bytes.
 *  @param [in]is_running - Flag used to skip triggering the background file
 *                          creation for debug contexts.
 *  @return Non-zero failure code, 0 on success.
 */
int lcz_event_manager_file_handler_build_file(uint8_t *absFilePath,
					      uint32_t *file_size,
					      bool is_running);

/** @brief Deletes the last created output log file.
 *  @return Non-zero failure code, 0 on success.
 */
int lcz_event_manager_file_handler_delete_file(void);

/** @brief Gets the count of events at the passed timestamp.
 *
 *  @param [in]timestamp - The timestamp where to look for events.
 *  @param [in]index - The zero based sub-index of the event.
 *  @param [out]count - The number of events that reside at this timestamp.
 *  @return Pointer to the requested event if found, NULL otherwise.
 */
SensorEvent_t *lcz_event_manager_file_handler_get_indexed_event_at_timestamp(
	uint32_t timestamp, uint16_t index, uint16_t *count);

/** @brief Gets the status of the last create log file request.
 *
 *  @return Details of the last log file create request.
 */
LogFileStatus_t lcz_event_manager_file_handler_get_log_file_status(void);

/** @brief Prepares a test event log for external use
 *
 * @param [in]dummy_log_file_properties - The dummy log file properties.
 * @param [out]log_path - The absolute path of the log file.
 * @param [out]log_file_size - The file size in bytes.
 * @param [in]is_running - Used to indicate runtime and debug contexts.
 * @return Zero for success, a non-zero error code otherwise.
 */
int lcz_event_manager_file_handler_build_test_file(
	DummyLogFileProperties_t *dummy_log_file_properties, uint8_t *log_path,
	uint32_t *log_file_size, bool is_running);

/** @brief Determines whether incoming events are stored to flash
 *
 * @param [in]save_to_flash - True when saving is required, false otherwise.
 */
void lcz_event_manager_file_handler_set_logging_state(bool save_to_flash);

/** @brief Resets the event manager file handler to factory settings
 *
 *         NOTE - This function assumes a software reset will follow. It
 *                disables the write to flash flag to hold off on adding
 *                new events to flash.
 */
void lcz_event_manager_file_handler_factory_reset(void);

#endif /* ifdef LCZ_EVENT_MANAGER_FILE_HANDLER_H */