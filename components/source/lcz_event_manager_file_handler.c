/*
 * @file lcz_event_manager_file_handler.c
 * @brief Event manager data handling.
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <logging/log.h>
#include "lcz_sensor_event.h"
#include "lcz_event_manager.h"
#include "lcz_event_manager_file_handler.h"
#include "lcz_sensor_event.h"
#include "file_system_utilities.h"
#include "lcz_qrtc.h"

LOG_MODULE_REGISTER(event_manager, CONFIG_LCZ_EVENT_MANAGER_LOG_LEVEL);

/*****************************************************************************/
/* Local Constant, Macro and Type Definitions                                */
/*****************************************************************************/
/* Data structure for internal use to keep track of file                     */
/* and timestamp indices                                                     */
typedef struct __lczEventManagerData_t {
	/* The index of the next event to be written.                        */
	uint16_t eventIndex;
	/* The sub-index last written - this gets incremented when a record  */
	/* is added with the same timestamp as the previous and reset when   */
	/* a new timestamp is used.                                          */
	uint16_t eventSubIndex;
	/* The last timestamp used when a record was saved. Used to          */
	/* determine if the sub index needs to be incremented or cleared.    */
	uint32_t lastEventTimestamp;
	/* The count of events in the event log                              */
	uint32_t eventCount;
	/* Indicates whether events should be stored in flash                */
	bool saving_enabled;
} lczEventManagerData_t;

/* This is the size of each file used to store private event data */
#define FILE_SIZE_BYTES                                                        \
	(CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE * sizeof(SensorEvent_t))

/* This is the total number of events available */
#define TOTAL_NUMBER_EVENTS                                                    \
	(CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES *                            \
	 CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE)

/* Internal file data structure used to store details of events and the      */
/* status of the files where the event data is stored.                       */
typedef struct {
	bool *pIsDirty;
	SensorEvent_t (*pFileData)[CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE];
} lczEventManagerFileData_t;

/* The filename prefix for private event manager files. These get suffixed   */
/* with a zero based index for the file.                                     */
#define LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_FILE_NAME "event_file_"

/* The filename prefix for output files read over user interfaces. The same  */
/* file name is always used so it can be deleted once the user acknowledges  */
/* it has been read out.                                                     */
#define LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME "event_file_out"

/* Timeout in ms to allow for getting the mutex before giving up when the
 * build file function is called
 */
#define LCZ_EVENT_MANAGER_BUILD_FILE_MUTEX_LOG_TIMEOUT_MS 100

/* Number of full loops over all events before exiting with an error that there
 * seems to be an issue with the data, prevents a possible lockup if more
 * events are requested than exist
 */
#define LCZ_EVENT_MANAGER_BUILD_FILE_FULL_LOOPS 3

/*****************************************************************************/
/* Local Data Definitions                                                    */
/*****************************************************************************/
/* Constructor for the above */
#define CONSTRUCTOR_LCZ_EVENT_MANAGER_FILE_DATA()                              \
	static bool dirtyFlags[CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES];      \
	static SensorEvent_t                                                   \
		eventData[CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES]            \
			 [CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE];           \
	static lczEventManagerFileData_t eventManagerFileData = { dirtyFlags,  \
								  eventData }

/* The instance of event file data that acts as a shadow of the content of   */
/* the event files stored to the file system. It also has flags used to      */
/* indicate when the shadow data needs to be written back to the files.      */
CONSTRUCTOR_LCZ_EVENT_MANAGER_FILE_DATA();

/* Instance of lczEventManagerData for event management. This is used to     */
/* track where in the shadow RAM data where we last added an event and other */
/* details of the event.                                                     */
static lczEventManagerData_t lczEventManagerData;

/* This is the mutex used to protect the shadow event log when it's          */
/* being updated.                                                            */
static struct k_mutex lczEventManagerFileHandlerMutex;

/* This is the stack used by the background thread used to update event log  */
/* data files.                                                               */
K_THREAD_STACK_DEFINE(lcz_event_manager_file_handler_stack_area,
		      CONFIG_LCZ_EVENT_MANAGER_BACKGROUND_THREAD_STACK_SIZE);

/* This is the background thread used to update data files stored to the     */
/* file system.                                                              */
static struct k_thread lcz_event_manager_file_handler_thread_data;

/* This is the message queue used to store events passed by callers.         */
static struct k_msgq lcz_event_manager_file_handler_msgq;

/* And the message queue buffer used to store messages                       */
static SensorEvent_t lcz_event_manager_file_handler_msgq_buffer
	[CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_QUEUE_SIZE];

/* This is the stack used by the work queue thread where event files are     */
/* saved                                                                     */
K_THREAD_STACK_DEFINE(lcz_event_manager_file_handler_workq_stack,
		      CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_THREAD_STACK_SIZE);

/* This is the work queue used to store files. Separate from the system work */
/* queue to avoid clogging it with potentially long storage operations.      */
static struct k_work_q lcz_event_manager_file_handler_workq;

/* And the work item to trigger background writes.                           */
static struct k_work lcz_event_manager_file_handler_work_item;

/* This is the status of the last request to create a log file.              */
static LogFileStatus_t log_file_status = LOG_FILE_STATUS_WAITING;

/* This is the work item used to prepare log files.                          */
static struct log_file_create_work_item_t {
	/* This is the work item used to manage the prepare operation */
	struct k_work work;
	/* This is the number of events to add */
	uint16_t event_count;
} log_file_create_work_item;

/* This is the work item used to prepare dummy log files.	              */
static struct dummy_log_file_create_work_item_t {
	/* This is the work item used to manage the prepare operation */
	struct k_work work;
	/* These are the dummy file properties */
	DummyLogFileProperties_t dummy_log_file_properties;
} dummy_log_file_create_work_item;

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
/* Background thread for file updating */
static void lcz_event_manager_file_handler_background_thread(void *unused1,
							     void *unused2,
							     void *unused3);

/* Work queue handler for background file update */
static void lcz_event_manager_file_handler_workq_handler(struct k_work *item);

/* Work queue handler for background log file creation */
static void
lcz_event_manager_file_handler_log_workq_handler(struct k_work *item);

/* Work queue handler for dummy log background file creation */
static void
lcz_event_manager_file_handler_dummy_log_workq_handler(struct k_work *item);

/* Gets the indexed event from the event structure */
static SensorEvent_t *
lcz_event_manager_file_handler_get_event(uint16_t eventIndex);

/* Finds the first empty record in the event log structure */
static int32_t
lcz_event_manager_file_handler_find_first_empty_event_by_type(void);

/* Finds the oldest record in the event log structure */
static int32_t
lcz_event_manager_file_handler_find_first_empty_event_by_age(void);

/* Finds the count of events in the event log */
static int32_t lcz_event_manager_file_handler_get_event_count(void);

/* Finds the next free record in the event log structure */
static int32_t lcz_event_manager_file_handler_find_next_free_event(void);

/* Finds the index of the oldest event in the log */
static uint32_t lcz_event_manager_file_handler_find_oldest_event(void);

/* Flags a page as needing to be saved in the background */
static void lcz_event_manager_file_handler_set_page_dirty(uint16_t eventIndex);

/* Loads files at startup */
static void lcz_event_manager_file_handler_load_files(void);

/* Saves any changed files */
static void lcz_event_manager_file_handler_save_files(void);

/* Determines where new events should be stored in the data structure */
static void lcz_event_manager_file_handler_get_indices(void);

/* Checks if all files are present and of the correct size */
static bool lcz_event_manager_file_handler_check_structure(void);

/* Rebuilds the file structure when files are not found or the wrong size */
static int lcz_event_manager_file_handler_rebuild_structure(void);

/* Finds an event at a sub-index */
static SensorEvent_t *lcz_event_manager_file_handler_get_subindexed_event(
	uint16_t startIndex, uint16_t subIndex, uint16_t count);

/* Adds a message read from the message queue to the event buffer */
static bool
lcz_event_manager_file_handler_add_event_private(SensorEvent_t *pSensorEvent);

/* Finds the first instance of an event going forward through the event log */
static int lcz_event_manager_file_handler_get_first_event_index_at_timestamp(
	uint32_t timestamp);

/* Finds the first instance of an event going backwards through the event log */
int lcz_event_manager_file_handler_get_last_event_index_at_timestamp(
	uint32_t timestamp);

/* Builds a log file in the background */
int lcz_event_manager_file_handler_background_build_file(uint16_t event_count);

/* Builds a dummy log file in the background */
int lcz_event_manager_file_handler_background_build_dummy_file(
	DummyLogFileProperties_t *dummy_log_file_properties);

/* Builds a dummy event added to dummy event files. */
void lcz_event_manager_file_handler_background_build_dummy_event(
	SensorEvent_t *sensor_event, uint32_t time_stamp,
	DummyLogFileProperties_t *dummy_log_file_properties,
	uint32_t event_data);

/* Module test code for the following */
/* Uncomment the following to enable module test */
/*#define LCZ_EVENT_MANAGER_FILE_HANDLER_UNIT_TEST*/
#ifdef LCZ_EVENT_MANAGER_FILE_HANDLER_UNIT_TEST
static uint32_t lcz_event_manager_file_handler_unit_test(void);
static void lcz_event_manager_file_handler_unit_test_delete_all_files(void);
static int
lcz_event_manager_file_handler_unit_test_create_file(uint8_t fileIndex,
						     uint16_t fileSize);
#endif

/*****************************************************************************/
/* Global Function Definitions                                               */
/*****************************************************************************/
void lcz_event_manager_file_handler_initialise(bool save_to_flash)
{
	/* Build the mutex we use to protect access to the event manager */
	/* file handler data structure                                   */
	k_mutex_init(&lczEventManagerFileHandlerMutex);

	/* And immediately lock it in case any threads bump this one */
	k_mutex_lock(&lczEventManagerFileHandlerMutex, K_FOREVER);

	/* The message queue used to store incoming events */
	k_msgq_init(&lcz_event_manager_file_handler_msgq,
		    (char *)lcz_event_manager_file_handler_msgq_buffer,
		    sizeof(SensorEvent_t),
		    CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_QUEUE_SIZE);

	/* Create the worker thread used to update the event log shadow */
	/* RAM via a work queue                                         */
	(void)k_thread_create(
		&lcz_event_manager_file_handler_thread_data,
		lcz_event_manager_file_handler_stack_area,
		K_THREAD_STACK_SIZEOF(
			lcz_event_manager_file_handler_stack_area),
		lcz_event_manager_file_handler_background_thread, NULL, NULL,
		NULL, CONFIG_LCZ_EVENT_MANAGER_BACKGROUND_THREAD_PRIORITY, 0,
		K_NO_WAIT);

	/* Set up the work item that will be used to trigger background */
	/* file writes                                                  */
	k_work_init(&lcz_event_manager_file_handler_work_item,
		    lcz_event_manager_file_handler_workq_handler);

	/* And the work item used to prepare log files as a background task */
	k_work_init(&log_file_create_work_item.work,
		    lcz_event_manager_file_handler_log_workq_handler);

	/* This is the work item used to prepare dummy log files as a
	   background task
	 */
	k_work_init(&dummy_log_file_create_work_item.work,
		    lcz_event_manager_file_handler_dummy_log_workq_handler);

	/* Start the work queue used to save event files */
	k_work_queue_start(
		&lcz_event_manager_file_handler_workq,
		lcz_event_manager_file_handler_workq_stack,
		K_THREAD_STACK_SIZEOF(
			lcz_event_manager_file_handler_workq_stack),
		CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_THREAD_PRIORITY, NULL);

	/* Check if all files are present and of the right size */
	if (!lcz_event_manager_file_handler_check_structure()) {
		/* If not they need to be rebuilt */
		lcz_event_manager_file_handler_rebuild_structure();
	}
	/* Now load the event files */
	lcz_event_manager_file_handler_load_files();
	/* And setup indexing so we know where to write next */
	lcz_event_manager_file_handler_get_indices();
	/* Store the flash saving enabled flag for later */
	lczEventManagerData.saving_enabled = save_to_flash;

	/* Safe to release resources now */
	k_mutex_unlock(&lczEventManagerFileHandlerMutex);
}

void lcz_event_manager_file_handler_add_event(
	SensorEventType_t sensorEventType, SensorEventData_t *pSensorEventData,
	uint32_t timestamp)
{
	SensorEvent_t sensorEvent = { 0 };

	/* Fill in the event details */
	sensorEvent.type = sensorEventType;
	*&sensorEvent.data = *pSensorEventData;
	sensorEvent.timestamp = timestamp;

	/* Then add to the event queue */
	(void)k_msgq_put(&lcz_event_manager_file_handler_msgq, &sensorEvent,
			 K_NO_WAIT);
}

int lcz_event_manager_file_handler_build_file(uint8_t *absFilePath,
					      uint32_t *file_size,
					      bool is_running)
{
	int result = -EBUSY;

	/* Lock resources whilst we check the file and event status */
	if (k_mutex_lock(
		    &lczEventManagerFileHandlerMutex,
		    K_MSEC(LCZ_EVENT_MANAGER_BUILD_FILE_MUTEX_LOG_TIMEOUT_MS)) !=
	    0) {
		/* Could not lock mutex */
		return -EDEADLK;
	}

	/* Is a log file creation request already in progress? */
	if (log_file_status != LOG_FILE_STATUS_PREPARING) {
		/* No so we can go ahead and create the file */
		log_file_status = LOG_FILE_STATUS_PREPARING;

		/* This will be the file path. */
		sprintf(absFilePath, "%s%s",
			CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
			LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME);
		/* The number of events that will be added */
		*file_size =
			sizeof(SensorEvent_t) * lczEventManagerData.eventCount;
		/* Set the event count in the work item so we know how many
		   events to add to the log file.
		 */
		log_file_create_work_item.event_count =
			lczEventManagerData.eventCount;
		/* Then trigger the background operation if we're running */
		if (is_running) {
			k_work_submit_to_queue(
				&lcz_event_manager_file_handler_workq,
				&log_file_create_work_item.work);
		}
		/* Background process started OK */
		result = 0;
	}

	/* OK to release resources now */
	k_mutex_unlock(&lczEventManagerFileHandlerMutex);

	/* If we're running (not unit testing), trigger a file update for
	 * any events that have been read out.
	 */
	if (is_running) {
		k_work_submit_to_queue(
			&lcz_event_manager_file_handler_workq,
			&lcz_event_manager_file_handler_work_item);
	}

	/* Then exit with our result */
	return (result);
}

int lcz_event_manager_file_handler_delete_file(void)
{
	int result;

	result = fsu_delete_files(
		CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
		LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME);

	return (result);
}

SensorEvent_t *lcz_event_manager_file_handler_get_indexed_event_at_timestamp(
	uint32_t timestamp, uint16_t index, uint16_t *count)
{
	uint16_t eventIndex;
	SensorEvent_t *pSensorEvent = NULL;
	uint16_t eventCount = 0;
	bool allEventsFound = false;
	uint16_t eventStartIndex;
	uint32_t startIndex;
	uint32_t endIndex = -EINVAL;

	/* Lock resources whilst we look for the event */
	k_mutex_lock(&lczEventManagerFileHandlerMutex, K_FOREVER);
	/* Get the first index */
	startIndex =
		lcz_event_manager_file_handler_get_first_event_index_at_timestamp(
			timestamp);
	/* Start index OK? */
	if (startIndex >= 0) {
		/* Is it zero? If so, we may have a wrap around condition */
		/* where events were added at the end of the log. If so,  */
		/* we need to check for this here.                        */
		if (startIndex == 0) {
			endIndex =
				lcz_event_manager_file_handler_get_last_event_index_at_timestamp(
					timestamp);
		}
	}
	/* Check start index again before proceeding */
	if (startIndex >= 0) {
		/* Is there an end index? */
		if (endIndex >= 0) {
			/* Is is the same as the start index? */
			if (startIndex == endIndex) {
				/* Yes, only one event here */
				eventCount = 1;
				eventStartIndex = startIndex;
				allEventsFound = true;
			} else {
				/* Is the last at the end of the log? */
				if (endIndex == (TOTAL_NUMBER_EVENTS - 1)) {
					/* Yes, first log is at the end */
					eventStartIndex = endIndex;
				} else {
					eventStartIndex = startIndex;
				}
			}
		} else {
			/* No, so start searching from the start index */
			eventStartIndex = startIndex;
		}
	}
	/* Only proceed here if we have a startIndex and the event hasn't    */
	/* been found.                                                       */
	if ((startIndex >= 0) && (allEventsFound == false)) {
		eventIndex = eventStartIndex;
		/* OK to count events now */
		while (allEventsFound == false) {
			if (lcz_event_manager_file_handler_get_event(eventIndex)
				    ->timestamp == timestamp) {
				eventCount++;
				eventIndex++;
				if (eventIndex == TOTAL_NUMBER_EVENTS) {
					eventIndex = 0;
				}
			} else {
				allEventsFound = true;
			}
		}
	}
	/* Do we need to return an event here? */
	if ((eventCount > 0) && (index < eventCount)) {
		/* Yes, now find the event by its index */
		pSensorEvent =
			lcz_event_manager_file_handler_get_subindexed_event(
				eventStartIndex, index, eventCount);
		/* Store the count of events for later */
		*count = eventCount;
	}
	/* OK to release resources now */
	k_mutex_unlock(&lczEventManagerFileHandlerMutex);
	return (pSensorEvent);
}

LogFileStatus_t lcz_event_manager_file_handler_get_log_file_status(void)
{
	/* Just exit with the last log file status */
	return (log_file_status);
}

int lcz_event_manager_file_handler_build_test_file(
	DummyLogFileProperties_t *dummy_log_file_properties, uint8_t *log_path,
	uint32_t *log_file_size, bool is_running)
{
	int result = -EBUSY;

	/* Lock resources whilst we check the file and event status */
	k_mutex_lock(&lczEventManagerFileHandlerMutex, K_FOREVER);

	/* Is a log file creation request already in progress? */
	if (log_file_status != LOG_FILE_STATUS_PREPARING) {
		/* No so we can go ahead and create the file */
		log_file_status = LOG_FILE_STATUS_PREPARING;
		/* This will be the file path. */
		sprintf(log_path, "%s%s",
			CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
			LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME);
		/* The number of events that will be added */
		*log_file_size = sizeof(SensorEvent_t) *
				 dummy_log_file_properties->event_count;
		/* Copy across the dummy file properties for use later */
		memcpy(&dummy_log_file_create_work_item
				.dummy_log_file_properties,
		       dummy_log_file_properties,
		       sizeof(DummyLogFileProperties_t));
		/* Then trigger the background operation if we're running.
		   If is_running is false, this in a unit test, so we don't 
		   want to trigger the background write as that function is
		   tested separately.
		 */
		if (is_running) {
			k_work_submit_to_queue(
				&lcz_event_manager_file_handler_workq,
				&dummy_log_file_create_work_item.work);
		}
		/* Background process started OK */
		result = 0;
	}

	/* OK to release resources now */
	k_mutex_unlock(&lczEventManagerFileHandlerMutex);

	/* Then exit with our result */
	return (result);
}

void lcz_event_manager_file_handler_set_logging_state(bool save_to_flash)
{
	/* Lock resources whilst we set the save enabled flag */
	k_mutex_lock(&lczEventManagerFileHandlerMutex, K_FOREVER);
	/* Set the save enabled flag state */
	lczEventManagerData.saving_enabled = save_to_flash;
	/* If saving has been enabled, we need to kick off a
	 * save here to add any new events to flash.
	 */
	if (save_to_flash) {
		k_work_submit_to_queue(
			&lcz_event_manager_file_handler_workq,
			&lcz_event_manager_file_handler_work_item);
	}
	/* OK to release resources now */
	k_mutex_unlock(&lczEventManagerFileHandlerMutex);
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
/** @brief The Event Manager File Handler background thread. Commits incoming
 *         events to the shadow of the event log then triggers a background
 *         write operation.
 *
 *  @param [in]unused1 - Unused parameter.
 *  @param [in]unused2 - Unused parameter.
 *  @param [in]unused3 - Unused parameter.
 */
static void lcz_event_manager_file_handler_background_thread(void *unused1,
							     void *unused2,
							     void *unused3)
{
	/* Kernel operation results */
	int result = 0;
	/* The last event read out of the queue */
	SensorEvent_t sensorEvent;
	/* The count of events needing to be added to the event log */
	uint16_t eventCount;
	/* Result of event add operation */
	bool addResult;

	/* Start the main event manager file handler loop */
	while (1) {
		/* Wait for the next event to arrive */
		result = k_msgq_get(&lcz_event_manager_file_handler_msgq,
				    &sensorEvent, K_FOREVER);

		/* We know we have at least one event here, but check   */
		/* if any more have arrived in between                  */
		eventCount = k_msgq_num_used_get(
			&lcz_event_manager_file_handler_msgq);
		/* We always have one event to have reached here */
		eventCount++;

		/* Lock resources whilst making changes */
		k_mutex_lock(&lczEventManagerFileHandlerMutex, K_FOREVER);

		/* Add new events to the event buffer */
		do {
			/* Add the current event */
			addResult = lcz_event_manager_file_handler_add_event_private(
				&sensorEvent);
			/* Only update stats if added OK */
			if (addResult) {
				/* Another event added */
				eventCount--;
				/* Check if there are any more */
				if (eventCount) {
					result = k_msgq_get(
						&lcz_event_manager_file_handler_msgq,
						&sensorEvent, K_FOREVER);
				}
			}
		} while ((eventCount) && (result == 0) && (addResult));

		/* Release resources after all changes are made */
		k_mutex_unlock(&lczEventManagerFileHandlerMutex);

		/* Then trigger a background write operation if
		 * saving to flash is enabled.
		 */
		if (lczEventManagerData.saving_enabled) {
			k_work_submit_to_queue(
				&lcz_event_manager_file_handler_workq,
				&lcz_event_manager_file_handler_work_item);
		}
	}
}

/** @brief Event Manager File Handler work queue processing for file strorage.
 *
 *  @param [in]item - Unused work item.
 */
static void lcz_event_manager_file_handler_workq_handler(struct k_work *item)
{
	/* Lock resources whilst making changes */
	k_mutex_lock(&lczEventManagerFileHandlerMutex, K_FOREVER);

	/* Save any changed files */
	lcz_event_manager_file_handler_save_files();

	/* Release resources after all changes are made */
	k_mutex_unlock(&lczEventManagerFileHandlerMutex);
}

/** @brief Event Manager File Handler work queue processing for log file
 *         creation.
 *
 *  @param [in]item - Work item containing details of the number of events to
 *                    add to the file.
 */
static void
lcz_event_manager_file_handler_log_workq_handler(struct k_work *item)
{
	int result;

	struct log_file_create_work_item_t *p_log_file_create_work_item =
		CONTAINER_OF(item, struct log_file_create_work_item_t, work);

	/* Lock resources whilst making changes */
	k_mutex_lock(&lczEventManagerFileHandlerMutex, K_FOREVER);

	/* Build the log file */
	result = lcz_event_manager_file_handler_background_build_file(
		p_log_file_create_work_item->event_count);

	/* Update log file status accordingly */
	if (result == 0) {
		log_file_status = LOG_FILE_STATUS_READY;
	} else {
		log_file_status = LOG_FILE_STATUS_FAILED;
	}

	/* Release resources after all changes are made */
	k_mutex_unlock(&lczEventManagerFileHandlerMutex);
}

/** @brief Event Manager File Handler work queue processing for dummy log file
 *         creation.
 *
 *  @param [in]item - Work item containing details of the number of events to
 *                    add to the file.
 */
static void
lcz_event_manager_file_handler_dummy_log_workq_handler(struct k_work *item)
{
	int result;

	struct dummy_log_file_create_work_item_t
		*p_dummy_log_file_create_work_item = CONTAINER_OF(
			item, struct dummy_log_file_create_work_item_t, work);

	/* Lock resources whilst making changes */
	k_mutex_lock(&lczEventManagerFileHandlerMutex, K_FOREVER);

	/* Build the log file */
	result = lcz_event_manager_file_handler_background_build_dummy_file(
		&p_dummy_log_file_create_work_item->dummy_log_file_properties);

	/* Update log file status accordingly */
	if (result == 0) {
		log_file_status = LOG_FILE_STATUS_READY;
	} else {
		log_file_status = LOG_FILE_STATUS_FAILED;
	}

	/* Release resources after all changes are made */
	k_mutex_unlock(&lczEventManagerFileHandlerMutex);
}

/** @brief Retrieves an event from the event log.
 *
 *  @param [in]eventIndex - The absolute event index.
 *
 *  @returns SensorEvent_t * - Pointer to the sensor event, NULL if not found.
 */
static SensorEvent_t *
lcz_event_manager_file_handler_get_event(uint16_t eventIndex)
{
	SensorEvent_t *sensorEvent = (SensorEvent_t *)NULL;
	uint16_t fileIndex;
	uint16_t fileEventIndex;

	if (eventIndex < TOTAL_NUMBER_EVENTS) {
		/* First get the index of the file where the event resides */
		fileIndex =
			eventIndex / CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE;
		/* Then the index within the file */
		fileEventIndex =
			eventIndex -
			(CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE * fileIndex);
		/* Then get the event location */
		sensorEvent = eventManagerFileData.pFileData[fileIndex] +
			      fileEventIndex;
	}
	return (sensorEvent);
}

/** @brief Gets the absolute index of the next free event. If the absolute
 *         index can't be found, there isn't a free slot and the oldest
 *         event must be found so we can start to overwrite existing events.
 *
 *  @returns The next free event absolute index, -1 if not found.
 */
static int32_t
lcz_event_manager_file_handler_find_first_empty_event_by_type(void)
{
	/* Assume there are no free records */
	int32_t result = -1;
	uint16_t eventIndex;
	SensorEvent_t *sensorEvent;

	/* Look for the first record with a type of RESERVED */
	for (eventIndex = 0;
	     (eventIndex < TOTAL_NUMBER_EVENTS) && (result == -1);) {
		/* Check the next record */
		sensorEvent =
			lcz_event_manager_file_handler_get_event(eventIndex);
		if (sensorEvent != NULL) {
			if (sensorEvent->type == SENSOR_EVENT_RESERVED) {
				/* Found it */
				result = eventIndex;
			}
		}
		/* Check the next */
		eventIndex++;
	}
	return (result);
}

/** @brief Gets the absolute index of the event with the oldest timestamp.
 *         This is used when the event log is full and we need to overwrite
 *         existing events.
 *
 *  @returns The absolute index of the event with the oldest timestamp.
 */
static int32_t
lcz_event_manager_file_handler_find_first_empty_event_by_age(void)
{
	/* Assume the first record is the newest */
	int32_t result = 0;
	uint16_t eventIndex;
	bool eventFound = false;
	uint32_t lastTimestamp;
	SensorEvent_t *sensorEvent;

	lastTimestamp = lcz_event_manager_file_handler_get_event(0)->timestamp;

	/* If all events get checked and we don't find a match, it */
	/* means (highly unlikely) all events are in chronological */
	/* order and the first one is the oldest. We finish this loop */
	/* with result left at 0 in this case */
	for (eventIndex = 1;
	     (eventIndex < TOTAL_NUMBER_EVENTS) && (eventFound == false);) {
		/* Get the next event */
		sensorEvent =
			lcz_event_manager_file_handler_get_event(eventIndex);

		/* Compare this and the previous event's timestamps */
		/* Is the previous newer than the next? */
		if (lastTimestamp > sensorEvent->timestamp) {
			/* Yes - Found the oldest, here's where we */
			/* start to overwrite events */
			eventFound = true;
			result = eventIndex;
		} else {
			/* No, so store the details of the current event */
			/* timestamp and check the next */
			lastTimestamp = sensorEvent->timestamp;
			eventIndex++;
		}
	}
	return (result);
}

/** @brief Gets the count of events in the event log.
 *
 *  @returns The count of events.
 */
static int32_t lcz_event_manager_file_handler_get_event_count(void)
{
	/* Assume the log is empty */
	int32_t result = 0;
	uint32_t eventIndex;

	/* Now check subsequent events */
	for (eventIndex = 0; eventIndex < TOTAL_NUMBER_EVENTS; eventIndex++) {
		if (lcz_event_manager_file_handler_get_event(eventIndex)->type !=
		    SENSOR_EVENT_RESERVED) {
			result++;
		}
	}
	return (result);
}

/** @brief Gets the absolute index of the next free event in the event log.
 *         This is either the first empty slot found, or the event with the
 *         oldest timestamp in event of the event log being full.
 *
 *  @returns The absolute index of the next free event.
 */
static int32_t lcz_event_manager_file_handler_find_next_free_event(void)
{
	int32_t result;

	/* First check for empty records */
	result =
		lcz_event_manager_file_handler_find_first_empty_event_by_type();
	/* Did we find one? */
	if (result == -1) {
		/* No, so check via timestamp age */
		result =
			lcz_event_manager_file_handler_find_first_empty_event_by_age();
	}
	return (result);
}

/** @brief Finds the index of the oldest event in the event log. Finds the
 *         oldest timestamp in the event log and ensures that there are
 *         contiguous records after it.
 *
 *  @returns The index of the oldest log.
 */
static uint32_t lcz_event_manager_file_handler_find_oldest_event(void)
{
	uint32_t eventIndex = 0;
	uint32_t timestamp;
	uint32_t eventCount;
	SensorEvent_t *sensorEvent;
	bool eventFound = false;

	/* Find the first valid event */
	for (eventIndex = 0;
	     (eventIndex < TOTAL_NUMBER_EVENTS) && (eventFound == false);) {
		/* Get the next event */
		sensorEvent =
			lcz_event_manager_file_handler_get_event(eventIndex);

		/* Then check if the event type is valid */
		if (sensorEvent->type != SENSOR_EVENT_RESERVED) {
			timestamp = sensorEvent->timestamp;
			eventFound = true;
		} else {
			eventIndex++;
		}
	}
	if (eventFound == false) {
		/* Empty event set so start at 0 */
		eventIndex = 0;
	} else {
		/* Find the next event with an older timestamp */
		for (eventCount = eventIndex; eventCount < TOTAL_NUMBER_EVENTS;
		     eventCount++) {
			/* Get the next event */
			sensorEvent = lcz_event_manager_file_handler_get_event(
				eventCount);

			/* Ignore it if blank */
			if (sensorEvent->type != SENSOR_EVENT_RESERVED) {
				/* Is it an earlier event? */
				if (sensorEvent->timestamp < timestamp) {
					/* This is the new oldest event */
					timestamp = sensorEvent->timestamp;
					eventIndex = eventCount;
				}
			}
		}
	}
	return (eventIndex);
}

/** @brief Sets a page of event logger shadow memory as dirty when a new event
 *         is added to it.
 *
 *  @param [in]eventIndex - The absolute index of the event to flag the page
 *                          where it resides as needing to be saved.
 */
static void lcz_event_manager_file_handler_set_page_dirty(uint16_t eventIndex)
{
	uint16_t fileIndex;

	if (eventIndex < TOTAL_NUMBER_EVENTS) {
		/* Get the index of the file where the event resides */
		fileIndex =
			eventIndex / CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE;
		/* And set its dirty flag */
		eventManagerFileData.pIsDirty[fileIndex] = true;
	}
}

/** @brief Loads all files at startup.
 */
static void lcz_event_manager_file_handler_load_files(void)
{
	uint16_t fileIndex;
	uint8_t fileName[LCZ_EVENT_MANAGER_FILENAME_SIZE];

	for (fileIndex = 0;
	     fileIndex < CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES;
	     fileIndex++) {
		/* Get the next file name */
		sprintf(fileName, "%s%s%d",
			CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_DIRECTORY,
			LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_FILE_NAME,
			fileIndex);

		fsu_read_abs(fileName,
			     &eventManagerFileData.pFileData[fileIndex],
			     FILE_SIZE_BYTES);

		/* No files are dirty at startup */
		eventManagerFileData.pIsDirty[fileIndex] = false;
	}
}

/** @brief Saves any files marked as dirty.
 */
static void lcz_event_manager_file_handler_save_files(void)
{
	uint16_t fileIndex;
	uint8_t fileName[LCZ_EVENT_MANAGER_FILENAME_SIZE];
	ssize_t writeSize;

	/* Have any files changed? */
	for (fileIndex = 0;
	     fileIndex < CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES;
	     fileIndex++) {
		/* Check the next file's dirty flag */
		if (eventManagerFileData.pIsDirty[fileIndex]) {
			/* If it's dirty, save it */
			sprintf(fileName, "%s%s%d",
				CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_DIRECTORY,
				LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_FILE_NAME,
				fileIndex);

			writeSize = fsu_write_abs(
				fileName,
				&eventManagerFileData.pFileData[fileIndex],
				FILE_SIZE_BYTES);

			/* Was any data written ? */
			if (writeSize) {
				/* OK to clear the dirty flag now */
				eventManagerFileData.pIsDirty[fileIndex] =
					false;
			}
		}
	}
}

/** @brief Called during initialisation to determine what event should be
 *         written to next.
 */
static void lcz_event_manager_file_handler_get_indices(void)
{
	/* Get the count of events in the log. We need to do this first */
	/* because handling changes slightly if there are limited numbers */
	/* of events in the log */
	lczEventManagerData.eventCount =
		lcz_event_manager_file_handler_get_event_count();
	/* Get the index of the next free record */
	lczEventManagerData.eventIndex =
		lcz_event_manager_file_handler_find_next_free_event();
	/* Reset the sub index for duplicate event timestamps */
	lczEventManagerData.eventSubIndex = 0;
	/* And reset the timestamp */
	lczEventManagerData.lastEventTimestamp = 0;
}

/** @brief Checks if all Event Manager files are present and of the right size.
 *         If not, Rebuild should be called to rebuild the file structure.
 *
 *  @return True if the structure is OK, False otherwise.
 */
static bool lcz_event_manager_file_handler_check_structure(void)
{
	uint16_t fileIndex;
	uint8_t fileName[LCZ_EVENT_MANAGER_FILENAME_SIZE];
	bool result = true;
	struct fs_dirent *pDirectoryEntry;
	size_t fileCount;

	/* Check if the next file exists */
	for (fileIndex = 0;
	     (fileIndex < CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES) && (result);
	     fileIndex++) {
		/* Assume each pass will fail */
		result = false;

		/* Reset the file count each time we check the next file */
		fileCount = 0;

		/* Build the next file name */
		sprintf(fileName, "%s%d",
			LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_FILE_NAME,
			fileIndex);

		/* First get the details of the file */
		pDirectoryEntry = fsu_find(
			CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_DIRECTORY,
			fileName, &fileCount, FS_DIR_ENTRY_FILE);

		/* Is the file available? */
		if (pDirectoryEntry != NULL) {
			/* Is it the right size? */
			if (pDirectoryEntry->size == FILE_SIZE_BYTES) {
				/* This file is OK */
				result = true;
			}
			/* Always free up the directory entry before */
			/* checking the next */
			fsu_free_found(pDirectoryEntry);
		}
	}
	return (result);
}

/** @brief Builds/Rebuilds the Event Manager file structure. Will be called
 *         for new product but also when the file size changes. When called,
 *         existing files are deleted then recreated.
 *
 *  @return Non-zero failure code, 0 on success.
 */
static int lcz_event_manager_file_handler_rebuild_structure(void)
{
	uint16_t fileIndex;
	uint8_t fileName[LCZ_EVENT_MANAGER_FILENAME_SIZE];
	int result = 0;
	struct fs_dirent *pDirectoryEntry;
	size_t fileCount;

	/* Check if the next file exists */
	for (fileIndex = 0;
	     (fileIndex < CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES) &&
	     (result == 0);
	     fileIndex++) {
		/* Reset the filecount each time we process a file */
		fileCount = 0;

		/* Build the next file name */
		sprintf(fileName, "%s%d",
			LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_FILE_NAME,
			fileIndex);

		/* First get the details of the file */
		pDirectoryEntry = fsu_find(
			CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_DIRECTORY,
			fileName, &fileCount, FS_DIR_ENTRY_FILE);

		/* Is the file available? */
		if (pDirectoryEntry != NULL) {
			/* Yes, so delete it */
			fsu_delete_files(
				CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_DIRECTORY,
				fileName);
			/* And free up the directory entry */
			fsu_free_found(pDirectoryEntry);
		}

		/* Now create the file */
		sprintf(fileName, "%s%s%d",
			CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_DIRECTORY,
			LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_FILE_NAME,
			fileIndex);

		memset(eventManagerFileData.pFileData[fileIndex], 0x0,
		       FILE_SIZE_BYTES);

		eventManagerFileData.pIsDirty[fileIndex] = false;

		if (fsu_write_abs(fileName,
				  eventManagerFileData.pFileData[fileIndex],
				  FILE_SIZE_BYTES) != FILE_SIZE_BYTES) {
			/* Failed to create file */
			result = -EINVAL;
		}
	}
	return (result);
}

/** @brief Finds an event from its sub-index given the point where to start
 *         looking in the event list.
 *
 *  @param [in]startIndex - The index in the event list where to start looking.
 *  @param [in]subIndex - The sub index of the event to find.
 *  @param [in]count - The number of events at the same timestamp.
 *
 *  @return The event if found, NULL otherwise.
 */
static SensorEvent_t *lcz_event_manager_file_handler_get_subindexed_event(
	uint16_t startIndex, uint16_t subIndex, uint16_t count)
{
	uint16_t eventIndex = startIndex;
	uint16_t eventCount;
	SensorEvent_t *pSensorEvent = NULL;
	bool eventFound = false;

	for (eventCount = 0; (eventCount < count) && (eventFound == false);
	     eventCount++) {
		pSensorEvent =
			lcz_event_manager_file_handler_get_event(eventIndex);
		/* Check if the event is valid before proceeding */
		if (pSensorEvent != NULL) {
			/* Is this the sub-indexed event ? */
			if (pSensorEvent->salt != subIndex) {
				/* No, so move on to the next and beware of */
				/* wrap around */
				eventIndex++;
				if (eventIndex >= TOTAL_NUMBER_EVENTS) {
					eventIndex = 0;
				}
			} else {
				/* Found our event, OK to exit */
				eventFound = true;
			}
		} else {
			/* If the event index requested is invalid,
			 * break out here.
			 */
			eventFound = true;
		}
	}
	return (pSensorEvent);
}

/** @brief Adds an event to the event log.
 *
 *  @param [in]pSensorEvent - The event to add.
 *  @return True if added, false otherwise.
 */
bool lcz_event_manager_file_handler_add_event_private(
	SensorEvent_t *pSensorEvent)
{
	bool result = false;
	SensorEvent_t *pAddedSensorEvent = (SensorEvent_t *)NULL;

	/* Has this timestamp been used before? */
	if (pSensorEvent->timestamp != lczEventManagerData.lastEventTimestamp) {
		/* It's a new timestamp, so reset our sub index */
		lczEventManagerData.eventSubIndex = 0;
		/* And store the new timestamp for use later */
		lczEventManagerData.lastEventTimestamp =
			pSensorEvent->timestamp;
	}
	/* Now add the event */
	/* First get a reference to it */
	pAddedSensorEvent = lcz_event_manager_file_handler_get_event(
		lczEventManagerData.eventIndex);
	/* If either are NULL, exit here */
	if ((pAddedSensorEvent != NULL) && (pSensorEvent != NULL)) {
		/* Then set the event data */
		pAddedSensorEvent->type = pSensorEvent->type;
		*&pAddedSensorEvent->data = pSensorEvent->data;
		pAddedSensorEvent->timestamp = pSensorEvent->timestamp;

		/* And assume the next event will be at the same time */
		pAddedSensorEvent->salt = lczEventManagerData.eventSubIndex++;

		/* Set the page where the event resides as dirty for */
		/* saving later in the background */
		lcz_event_manager_file_handler_set_page_dirty(
			lczEventManagerData.eventIndex);

		/* Index the next event for writing later */
		lczEventManagerData.eventIndex++;
		/* Check for wrap around here */
		if (lczEventManagerData.eventIndex >= TOTAL_NUMBER_EVENTS) {
			/* Go back to the start of the list */
			lczEventManagerData.eventIndex = 0;
		}
		/* Update the count of events in the log */
		lczEventManagerData.eventCount++;
		if (lczEventManagerData.eventCount > TOTAL_NUMBER_EVENTS) {
			lczEventManagerData.eventCount = TOTAL_NUMBER_EVENTS;
		}
		/* Added OK */
		result = true;
	}
	return(result);
}

/** @brief Finds the first event with the passed timestamp in the event log
 *         going forwards.
 *
 *  @param [in]timestamp - The timestamp to find the event for.
 *
 *  @returns The index of the event, -EINVAL if not found.
 */
int lcz_event_manager_file_handler_get_first_event_index_at_timestamp(
	uint32_t timestamp)
{
	int32_t eventIndex;
	bool eventFound = false;
	SensorEvent_t *pSensorEvent = NULL;

	/* Read through the events and find a match */
	for (eventIndex = 0;
	     (eventIndex < TOTAL_NUMBER_EVENTS) && (eventFound == false);) {
		pSensorEvent =
			lcz_event_manager_file_handler_get_event(eventIndex);

		if (pSensorEvent->timestamp == timestamp) {
			eventFound = true;
		} else {
			eventIndex++;
		}
	}
	/* Flag if not found */
	if (eventFound == false) {
		eventIndex = -EINVAL;
	}
	return (eventIndex);
}

/** @brief Finds the first event with the passed timestamp in the event log
 *         going backwards.
 *
 *  @param [in]timestamp - The timestamp to find the event for.
 *
 *  @returns The index of the event, -EINVAL if not found.
 */
int lcz_event_manager_file_handler_get_last_event_index_at_timestamp(
	uint32_t timestamp)
{
	int32_t eventIndex;
	bool eventFound = false;
	SensorEvent_t *pSensorEvent = NULL;

	/* Read through the events and find a match */
	for (eventIndex = (TOTAL_NUMBER_EVENTS - 1);
	     (eventIndex > 0) && (eventFound == false);) {
		pSensorEvent =
			lcz_event_manager_file_handler_get_event(eventIndex);

		if (pSensorEvent->timestamp == timestamp) {
			eventFound = true;
		} else {
			eventIndex++;
		}
	}
	/* Flag if not found */
	if (eventFound == false) {
		eventIndex = -EINVAL;
	}
	return (eventIndex);
}

/** @brief Private method used to build log files as a background task.
 *
 *  @param [in]event_count - The number of events to add.
 *
 *  @returns The result of the operation, non-zero if not successful.
 */
int lcz_event_manager_file_handler_background_build_file(uint16_t event_count)
{
	uint8_t file_name[LCZ_EVENT_MANAGER_FILENAME_SIZE];
	int result = 0;
	uint32_t event_index;
	SensorEvent_t *pSensorEvent;
#if (CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_EVENT_BUFFER_SIZE > 1)
	SensorEvent_t sensor_event_buffer
		[CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_EVENT_BUFFER_SIZE];
	uint8_t buffer_index = 0;
	uint32_t buffer_indexes
		[CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_EVENT_BUFFER_SIZE];
#endif
	/* This is the number of events added to the file */
	uint32_t events_added = 0;
	uint8_t full_loops = 0;

	/* Build the empty output file */
	sprintf(file_name, "%s%s",
		CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
		LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME);

	/* Create as blank then append events to it */
	result = fsu_write_abs(file_name, NULL, 0);

	if (result == 0) {
		/* Now find the oldest event */
		event_index =
			lcz_event_manager_file_handler_find_oldest_event();
		/* Then continue adding to the file and deleting them */
		/* from the live list as we go */
		for (events_added = 0;
		     (events_added < event_count) && (result == 0);
		     events_added++) {
			/* Get the next event */
			pSensorEvent = lcz_event_manager_file_handler_get_event(
				event_index);

			/* Is it blank? */
			if (pSensorEvent->type != SENSOR_EVENT_RESERVED) {
#if (CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_EVENT_BUFFER_SIZE > 1)
				memcpy(&sensor_event_buffer[buffer_index],
				       pSensorEvent, sizeof(SensorEvent_t));
				buffer_indexes[buffer_index] = event_index;
				++buffer_index;

				/* Another event read out */
				lczEventManagerData.eventCount--;

				if (buffer_index ==
				    CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_EVENT_BUFFER_SIZE) {
					/* No, so add it to the file */
					if (fsu_append_abs(
						    file_name,
						    sensor_event_buffer,
						    sizeof(SensorEvent_t) *
							    buffer_index) !=
					    sizeof(SensorEvent_t) *
						    buffer_index) {
						/* Failed to update the file,
						 * should see the number of
						 * bytes written echoed back
						 */
						result = -EINVAL;
					}

					/* Update event indices only when added
					 * OK
					 */
					if (result == 0) {
						uint8_t i = 0;
						while (i < buffer_index) {
							/* Mark this event as
							 * free for use later
							 */
							memset(lcz_event_manager_file_handler_get_event(
								       buffer_indexes
									       [i]),
							       0x0,
							       sizeof(SensorEvent_t));

							/* Mark this page as
							 * needing to be saved
							 */
							lcz_event_manager_file_handler_set_page_dirty(
								buffer_indexes
									[i]);
							++i;
						}
					}

					buffer_index = 0;
				}
#else
				/* No, so add it to the file */
				if (fsu_append_abs(file_name, pSensorEvent,
						   sizeof(SensorEvent_t)) !=
				    sizeof(SensorEvent_t)) {
					/* Failed to update the file, should
					 * see the number of bytes written
					 * echoed back
					 */
					result = -EINVAL;
				}

				/* Update event indices only when added OK */
				if (result == 0) {
					/* Mark this event as free for use
					 * later
					 */
					memset(pSensorEvent, 0x0,
					       sizeof(SensorEvent_t));

					/* Another event read out */
					lczEventManagerData.eventCount--;

					/* Mark this page as needing to be
					 * saved
					 */
					lcz_event_manager_file_handler_set_page_dirty(
						event_index);
				}
#endif
			}

			if (result == 0) {
				/* Check next event */
				event_index++;

				/* Beware of wrap around here */
				if (event_index >= TOTAL_NUMBER_EVENTS) {
					event_index = 0;
					++full_loops;

					if (full_loops >=
					    LCZ_EVENT_MANAGER_BUILD_FILE_FULL_LOOPS) {
						/* Something is wrong here, all
						 * entries have been checked
						 * multiple times and there are
						 * insufficient entries, output
						 * this error and exit checking
						 * as there is nothing more
						 * that can be done
						 */
						result = -EIO;
						LOG_ERR("Expected %d events, "
							"failed after %d events"
							" with %d loops of all "
							"event entries",
							event_count,
							events_added,
							full_loops);
					}
				}
			}
		}
	}

#if (CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_EVENT_BUFFER_SIZE > 1)
	if (result == 0 && buffer_index > 0) {
		/* No, so add it to the file */
		if (fsu_append_abs(file_name, sensor_event_buffer,
				   sizeof(SensorEvent_t) * buffer_index) !=
		    sizeof(SensorEvent_t) * buffer_index) {
			/* Failed to update the file, should see the number of
			 * bytes written echoed back
			 */
			result = -EINVAL;
		}

		/* Update event indices only when added OK */
		if (result == 0) {
			uint8_t i = 0;
			while (i < buffer_index) {
				/* Mark this event as free for use later */
				memset(lcz_event_manager_file_handler_get_event(
					       buffer_indexes[i]),
				       0x0, sizeof(SensorEvent_t));

				/* Mark this page as needing to be saved */
				lcz_event_manager_file_handler_set_page_dirty(
					buffer_indexes[i]);
				++i;
			}
		}
	}
#endif

	/* Then exit with our result */
	return (result);
}

/** @brief Private method used to build dummy log files as a background task.
 *
 *  @param [in]dummy_log_file_properties - The properties of the dummy file.
 *
 *  @returns The result of the operation, non-zero if not successful.
 */
int lcz_event_manager_file_handler_background_build_dummy_file(
	DummyLogFileProperties_t *dummy_log_file_properties)
{
	uint8_t file_name[LCZ_EVENT_MANAGER_FILENAME_SIZE];
	int result = 0;
	uint32_t event_count;
	SensorEvent_t sensor_event = { 0 };
	/* This is the rolling timestamp value added to events */
	uint32_t time_stamp = dummy_log_file_properties->start_time_stamp;
	/* This is the rolling data value added to events */
	uint32_t event_data = 0;

	/* Build the empty output file */
	sprintf(file_name, "%s%s",
		CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
		LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME);

	/* Create as blank then append events to it */
	result = fsu_write_abs(file_name, NULL, 0);

	if (result == 0) {
		/* Now add our dummy events */
		for (event_count = 0;
		     (event_count < dummy_log_file_properties->event_count) &&
		     (result == 0);
		     event_count++) {
			/* Build the next event */
			lcz_event_manager_file_handler_background_build_dummy_event(
				&sensor_event, time_stamp,
				dummy_log_file_properties, event_data);
			/* Add it to the file */
			if (fsu_append_abs(file_name, &sensor_event,
					   sizeof(SensorEvent_t)) !=
			    sizeof(SensorEvent_t)) {
				/* Failed to update the file, should see */
				/* the number of bytes written echoed back */
				result = -EINVAL;
			}
			/* Update event details only when added OK */
			if (result == 0) {
				/* Boolean data alternates */
				if (dummy_log_file_properties->event_data_type ==
				    DUMMY_LOG_DATA_TYPE_BOOL) {
					event_data = !event_data;
				} else {
					/* All other data types increment */
					event_data++;
				}
				/* Does the salt need to be updated? */
				if (dummy_log_file_properties->update_rate ==
				    0) {
					sensor_event.salt++;
				} else {
					/* Update timestamp */
					time_stamp += dummy_log_file_properties
							      ->update_rate;
				}
			}
		}
	}
	/* Then exit with our result */
	return (result);
}

/** @brief Private method used to build dummy events added to dummy log files.
 *
 *  @param [out]sensor_event - The dummy event.
 *  @param [in]time_stamp - The timestamp to use.
 *  @param [in]dummy_log_file_properties - The dummy log file properties.
 *  @param [in]event_data - The data to use.
 *
 *  @returns The result of the operation, non-zero if not successful.
 */
void lcz_event_manager_file_handler_background_build_dummy_event(
	SensorEvent_t *sensor_event, uint32_t time_stamp,
	DummyLogFileProperties_t *dummy_log_file_properties,
	uint32_t event_data)
{
	/* Set timestamp */
	sensor_event->timestamp = time_stamp;
	/* Set event type */
	sensor_event->type = dummy_log_file_properties->event_type;
	/* Set data */
	switch (dummy_log_file_properties->event_data_type) {
	case (DUMMY_LOG_DATA_TYPE_U32):
		sensor_event->data.u32 = event_data;
		break;
	case (DUMMY_LOG_DATA_TYPE_FLOAT):
		sensor_event->data.f = ((float)(event_data));
		break;
	default:
		sensor_event->data.u16 = ((uint16_t)(event_data));
	}
}

#ifdef LCZ_EVENT_MANAGER_FILE_HANDLER_UNIT_TEST
/**@brief Module test code for the Lcz_Event_Manager_File_Handler.
 *
 * @retval A positive value indicating the test that failed, 0 for success.
 */
static uint32_t lcz_event_manager_file_handler_unit_test(void)
{
	int result = 0;
	int failResult = 0;
	uint16_t fileIndex;
	uint16_t eventIndex;
	uint8_t eventType;
	uint32_t timestamp;
	uint32_t data;
	SensorEvent_t *pSensorEvent;
	SensorEvent_t sensorEvent;
	uint16_t localEventIndex;
	uint8_t outputFileName[LCZ_EVENT_MANAGER_FILENAME_SIZE];
	uint8_t fileName[LCZ_EVENT_MANAGER_FILENAME_SIZE];
	struct fs_dirent *directoryEntry;
	size_t count;
	uint16_t eventCount;
	uint16_t badFileIndex;
	SensorEvent_t sensorEventReadback;
	uint32_t outputFileSize;
	DummyLogFileProperties_t dummy_log_properties;
	uint16_t salt;

#define TOTAL_FILE_SIZE_BYTES                                                  \
	(CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES * FILE_SIZE_BYTES)

#ifdef ENABLED

	/* lcz_event_manager_file_handler_get_event_count */

	/* Empty log returns 0 events */
	if (result == 0) {
		failResult++;
		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);
		if (lcz_event_manager_file_handler_get_event_count() != 0) {
			result = failResult;
		}
	}

	/* Full event log returns maximum number of events*/
	if (result == 0) {
		failResult++;
		memset(eventManagerFileData.pFileData, SENSOR_EVENT_TEMPERATURE,
		       TOTAL_FILE_SIZE_BYTES);
		if (lcz_event_manager_file_handler_get_event_count() !=
		    TOTAL_NUMBER_EVENTS) {
			result = failResult;
		}
	}
	/* First and last records in use returns 2 events */
	if (result == 0) {
		failResult++;

		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);
		eventManagerFileData.pFileData[0]->type =
			SENSOR_EVENT_TEMPERATURE;
		(eventManagerFileData
			 .pFileData[CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES -
				    1] +
		 CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE - 1)
			->type = SENSOR_EVENT_TEMPERATURE;
		if (lcz_event_manager_file_handler_get_event_count() != 2) {
			result = failResult;
		}
	}
	/* lcz_event_manager_file_handler_get_event */

	/* Each event should have an ascending timestamp and data value and */
	/* an event type increasing to the max available then rolling over  */
	/* when all event types are consumed.                               */
	if (result == 0) {
		failResult++;
		eventType = 0;
		timestamp = 0;
		data = 0;

		for (fileIndex = 0;
		     fileIndex < CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES;
		     fileIndex++) {
			for (eventIndex = 0;
			     eventIndex <
			     CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE;
			     eventIndex++) {
				/* Set the event details directly */
				eventData[fileIndex][eventIndex].timestamp =
					timestamp;
				eventData[fileIndex][eventIndex].data.u32 =
					data;
				eventData[fileIndex][eventIndex].type =
					(SensorEventType_t)eventType;
				/* Update the event type here */
				eventType++;
				if (eventType >=
				    (uint8_t)NUMBER_OF_SENSOR_EVENTS) {
					eventType = 0;
				}
				/* And the other data being written */
				timestamp++;
				data++;
			}
		}

		/* Now read back each event via the Get Event method */
		eventType = 0;
		timestamp = 0;
		data = 0;

		for (fileIndex = 0;
		     (fileIndex < CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES) &&
		     (result == 0);
		     fileIndex++) {
			for (eventIndex = 0;
			     (eventIndex <
			      CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE) &&
			     (result == 0);
			     eventIndex++) {
				/* Read back the next event */
				pSensorEvent = lcz_event_manager_file_handler_get_event(
					(fileIndex *
					 CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE) +
					eventIndex);
				/* Shouldn't be null */
				if (pSensorEvent != NULL) {
					/* Check members */
					if ((pSensorEvent->timestamp !=
					     timestamp) ||
					    (pSensorEvent->data.u32 != data) ||
					    (pSensorEvent->type != eventType)) {
						/* Something not right */
						result = failResult;
					}
					if (result == 0) {
						/* Update data for next event */
						eventType++;
						if (eventType >=
						    (uint8_t)
							    NUMBER_OF_SENSOR_EVENTS) {
							eventType = 0;
						}
						timestamp++;
						data++;
					}
				} else {
					result = failResult;
				}
			}
		}
	}

	/* lcz_event_manager_file_handler_find_first_empty_event_by_type */

	/* Should get 0 back for an empty record set */
	if (result == 0) {
		failResult++;
		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);
		if (lcz_event_manager_file_handler_find_first_empty_event_by_type() !=
		    0) {
			result = failResult;
		}
	}

	/* Should get back -1 when no free record is available */
	if (result == 0) {
		failResult++;
		/* Prefill with a non-zero event type */
		memset(eventManagerFileData.pFileData, SENSOR_EVENT_TEMPERATURE,
		       TOTAL_FILE_SIZE_BYTES);
		if (lcz_event_manager_file_handler_find_first_empty_event_by_type() !=
		    -1) {
			result = failResult;
		}
	}

	/* Should get back Last Record when Last record is free */
	if (result == 0) {
		failResult++;
		(eventManagerFileData
			 .pFileData[CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES -
				    1] +
		 (CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE - 1))
			->type = SENSOR_EVENT_RESERVED;
		if (lcz_event_manager_file_handler_find_first_empty_event_by_type() !=
		    TOTAL_NUMBER_EVENTS - 1) {
			result = failResult;
		}
	}

	/* lcz_event_manager_file_handler_find_first_empty_event_by_age */

	/* First record is the oldest */
	if (result == 0) {
		failResult++;
		timestamp = 0;
		for (fileIndex = 0;
		     fileIndex < CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES;
		     fileIndex++) {
			for (eventIndex = 0;
			     eventIndex <
			     CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE;
			     eventIndex++) {
				(eventManagerFileData.pFileData[fileIndex] +
				 eventIndex)
					->timestamp = timestamp++;
			}
		}
		if (lcz_event_manager_file_handler_find_first_empty_event_by_age() !=
		    0) {
			result = failResult;
		}
	}

	/* Last event is the oldest */
	if (result == 0) {
		failResult++;
		/* Start with a timestamp of 1 here so we can */
		/* set the last to 0 making it the oldest     */
		timestamp = 1;
		for (fileIndex = 0;
		     fileIndex < CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES;
		     fileIndex++) {
			for (eventIndex = 0;
			     eventIndex <
			     CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE;
			     eventIndex++) {
				(eventManagerFileData.pFileData[fileIndex] +
				 eventIndex)
					->timestamp = timestamp;
			}
		}
		(eventManagerFileData
			 .pFileData[CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES -
				    1] +
		 CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE - 1)
			->timestamp = 0;
		if (lcz_event_manager_file_handler_find_first_empty_event_by_age() !=
		    TOTAL_NUMBER_EVENTS - 1) {
			result = failResult;
		}
	}

	/* Middle record is the oldest */
	if (result == 0) {
		failResult++;
		/* Again start with a timestamp of 1 so we can     */
		/* set the middle one to zero making it the oldest */
		timestamp = 1;
		for (fileIndex = 0;
		     fileIndex < CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES;
		     fileIndex++) {
			for (eventIndex = 0;
			     eventIndex <
			     CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE;
			     eventIndex++) {
				(eventManagerFileData.pFileData[fileIndex] +
				 eventIndex)
					->timestamp = timestamp++;
			}
		}
		(eventManagerFileData
			 .pFileData[CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES /
				    2] +
		 CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE / 2)
			->timestamp = 0;

		if (CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES == 1) {
			eventIndex =
				(CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE / 2);
		} else {
			eventIndex =
				((CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES *
				  CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE) /
				 2) +
				(CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE / 2);
		}

		if (lcz_event_manager_file_handler_find_first_empty_event_by_age() !=
		    eventIndex) {
			result = failResult;
		}
	}

	/* All records blank should start with the first record */
	if (result == 0) {
		failResult++;
		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);
		if (lcz_event_manager_file_handler_find_first_empty_event_by_age() !=
		    0) {
			result = failResult;
		}
	}

	/* Fill with records with ascending timestamp values and make sure */
	/* the oldest is found */
	if (result == 0) {
		failResult++;
		/* Start with a timestamp of 1 here */
		timestamp = 1;
		for (fileIndex = 0;
		     fileIndex < CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES;
		     fileIndex++) {
			for (eventIndex = 0;
			     eventIndex <
			     CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE;
			     eventIndex++) {
				(eventManagerFileData.pFileData[fileIndex] +
				 eventIndex)
					->timestamp = timestamp++;
			}
		}
		if (lcz_event_manager_file_handler_find_first_empty_event_by_age() !=
		    0) {
			result = failResult;
		}
	}

	/* lcz_event_manager_file_handler_set_page_dirty */

	/* First indexed event of each page should set appropriate dirty flag */
	if (result == 0) {
		failResult++;

		for (fileIndex = 0;
		     (fileIndex < CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES) &&
		     (result == 0);
		     fileIndex++) {
			memset(eventManagerFileData.pIsDirty, 0x0,
			       CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES);

			eventIndex = fileIndex *
				     CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE;
			lcz_event_manager_file_handler_set_page_dirty(
				eventIndex);
			if (!eventManagerFileData.pIsDirty[fileIndex]) {
				result = failResult;
			}
		}
	}
	/* Last indexed event of each page should set the appropriate dirty flag */
	if (result == 0) {
		failResult++;

		for (fileIndex = 0;
		     (fileIndex < CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES) &&
		     (result == 0);
		     fileIndex++) {
			memset(eventManagerFileData.pIsDirty, 0x0,
			       CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES);

			eventIndex =
				(fileIndex *
				 CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE) +
				(CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE - 1);
			lcz_event_manager_file_handler_set_page_dirty(
				eventIndex);
			if (!eventManagerFileData.pIsDirty[fileIndex]) {
				result = failResult;
			}
		}
	}

	/* Check file storage and recall */

	/* lcz_event_manager_file_handler_check_structure */
	/* Get false when there are no files present */
	if (result == 0) {
		failResult++;
		lcz_event_manager_file_handler_unit_test_delete_all_files();

		if (lcz_event_manager_file_handler_check_structure() != false) {
			result = failResult;
		}
	}

	/* Get false when any file is not present */
	if (result == 0) {
		failResult++;
		for (badFileIndex = 0;
		     (badFileIndex <
		      CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES) &&
		     (result == 0);
		     badFileIndex++) {
			lcz_event_manager_file_handler_unit_test_delete_all_files();

			for (fileIndex = 0;
			     (fileIndex <
			      CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES) &&
			     (result == 0);
			     fileIndex++) {
				if (fileIndex != badFileIndex) {
					lcz_event_manager_file_handler_unit_test_create_file(
						fileIndex, FILE_SIZE_BYTES);
				}
			}
			/* Now check the structure and make sure we get false */
			if (lcz_event_manager_file_handler_check_structure() !=
			    false) {
				result = failResult;
			}
		}
	}
	/* Get false when any file is not the right size */
	if (result == 0) {
		failResult++;
		for (badFileIndex = 0;
		     (badFileIndex <
		      CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES) &&
		     (result == 0);
		     badFileIndex++) {
			lcz_event_manager_file_handler_unit_test_delete_all_files();

			for (fileIndex = 0;
			     (fileIndex <
			      CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES) &&
			     (result == 0);
			     fileIndex++) {
				if (fileIndex != badFileIndex) {
					lcz_event_manager_file_handler_unit_test_create_file(
						fileIndex, FILE_SIZE_BYTES);
				} else {
					lcz_event_manager_file_handler_unit_test_create_file(
						fileIndex, FILE_SIZE_BYTES - 1);
				}
			}
			/* Now check the structure and make sure we get false */
			if (lcz_event_manager_file_handler_check_structure() !=
			    false) {
				result = failResult;
			}
		}
	}
	/* Get true when all files are present and the right size */
	if (result == 0) {
		failResult++;

		lcz_event_manager_file_handler_unit_test_delete_all_files();

		for (fileIndex = 0;
		     (fileIndex < CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES) &&
		     (result == 0);
		     fileIndex++) {
			lcz_event_manager_file_handler_unit_test_create_file(
				fileIndex, FILE_SIZE_BYTES);
		}
		/* Now check the structure and make sure we get true */
		if (lcz_event_manager_file_handler_check_structure() != true) {
			result = failResult;
		}
	}

	/* lcz_event_manager_file_handler_rebuild_structure */
	/* Check all files are created, blank and the right size */
	if (result == 0) {
		failResult++;

		lcz_event_manager_file_handler_unit_test_delete_all_files();

		for (fileIndex = 0;
		     (fileIndex < CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES) &&
		     (result == 0);
		     fileIndex++) {
			/* Add an offset of 1 here for test cases for 1 file */
			lcz_event_manager_file_handler_unit_test_create_file(
				fileIndex, FILE_SIZE_BYTES + fileIndex + 1);
		}

		/* We should get a false here */
		if (lcz_event_manager_file_handler_check_structure() != false) {
			result = failResult;
		}
		if (result == 0) {
			lcz_event_manager_file_handler_rebuild_structure();

			/* And this should give us a true */
			if (lcz_event_manager_file_handler_check_structure() !=
			    true) {
				result = failResult;
			}
		}
	}

	/* lcz_event_manager_file_handler_save_files */
	/* lcz_event_manager_file_handler_load_files */

	/* Save and recall a blank data set */
	if (result == 0) {
		failResult++;

		lcz_event_manager_file_handler_unit_test_delete_all_files();
		lcz_event_manager_file_handler_rebuild_structure();

		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);
		memset(eventManagerFileData.pIsDirty, true,
		       CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES);

		lcz_event_manager_file_handler_save_files();

		/* No data should be dirty after saving */
		for (fileIndex = 0;
		     (fileIndex < CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES) &&
		     (result == 0);
		     fileIndex++) {
			if (eventManagerFileData.pIsDirty[fileIndex]) {
				result = failResult;
			}
		}

		if (result == 0) {
			lcz_event_manager_file_handler_load_files();

			/* Now check each event */
			for (eventIndex = 0;
			     (eventIndex < TOTAL_NUMBER_EVENTS) &&
			     (result == 0);
			     eventIndex++) {
				pSensorEvent =
					lcz_event_manager_file_handler_get_event(
						eventIndex);
				if ((pSensorEvent->data.u32 != 0) ||
				    (pSensorEvent->reserved1 != 0) ||
				    (pSensorEvent->reserved2 != 0) ||
				    (pSensorEvent->salt != 0) ||
				    (pSensorEvent->timestamp != 0) ||
				    (pSensorEvent->type != 0)) {
					result = failResult;
				}
			}
		}
	}

#endif

	/* Save and recall for an incrementing data set */
	if (result == 0) {
		failResult++;

		lcz_event_manager_file_handler_unit_test_delete_all_files();
		lcz_event_manager_file_handler_rebuild_structure();

		for (eventIndex = 0, eventType = 0;
		     eventIndex < TOTAL_NUMBER_EVENTS;
		     eventIndex++, eventType++) {
			fileIndex = eventIndex /
				    CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE;
			localEventIndex =
				eventIndex -
				(fileIndex *
				 CONFIG_LCZ_EVENT_MANAGER_EVENTS_PER_FILE);
			pSensorEvent =
				(eventManagerFileData.pFileData[fileIndex] +
				 localEventIndex);
			pSensorEvent->data.u32 = eventIndex;
			pSensorEvent->reserved1 = eventType;
			pSensorEvent->reserved2 = eventType + 1;
			pSensorEvent->salt = eventType + 2;
			pSensorEvent->timestamp = eventIndex + 3;
			pSensorEvent->type = eventType + 4;
		}
		memset(eventManagerFileData.pIsDirty, true,
		       CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES);

		lcz_event_manager_file_handler_save_files();

		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);

		/* No data should be dirty after saving */
		for (fileIndex = 0;
		     (fileIndex < CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES) &&
		     (result == 0);
		     fileIndex++) {
			if (eventManagerFileData.pIsDirty[fileIndex]) {
				result = failResult;
			}
		}

		if (result == 0) {
			lcz_event_manager_file_handler_load_files();

			for (eventIndex = 0, eventType = 0;
			     (eventIndex < TOTAL_NUMBER_EVENTS) &&
			     (result == 0);
			     eventIndex++, eventType++) {
				if (eventIndex == 0xFC) {
					__NOP();
				}

				pSensorEvent =
					lcz_event_manager_file_handler_get_event(
						eventIndex);
				if ((pSensorEvent->data.u32 != eventIndex) ||
				    (pSensorEvent->reserved1 != eventType) ||
				    (pSensorEvent->reserved2 !=
				     (uint8_t)(eventType + 1)) ||
				    (pSensorEvent->salt !=
				     (uint8_t)(eventType + 2)) ||
				    (pSensorEvent->timestamp !=
				     eventIndex + 3) ||
				    (pSensorEvent->type !=
				     (uint8_t)(eventType + 4))) {
					result = failResult;
					pSensorEvent = NULL;
				}
			}
		}
	}

	/* lcz_event_manager_file_handler_add_event_private */
	/* The following are for startup conditions */
	/* Add a single event to a blank file set and readback file and check indices */
	if (result == 0) {
		failResult++;

		lcz_event_manager_file_handler_unit_test_delete_all_files();
		lcz_event_manager_file_handler_rebuild_structure();
		lcz_event_manager_file_handler_get_indices();

		sensorEvent.data.u32 = 1;
		sensorEvent.type = 1;
		sensorEvent.timestamp = 1;

		lcz_event_manager_file_handler_add_event_private(&sensorEvent);

		lcz_event_manager_file_handler_save_files();

		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);

		lcz_event_manager_file_handler_load_files();
		lcz_event_manager_file_handler_get_indices();

		for (eventIndex = 0;
		     (eventIndex < TOTAL_NUMBER_EVENTS) && (result == 0);
		     eventIndex++) {
			pSensorEvent = lcz_event_manager_file_handler_get_event(
				eventIndex);

			/* Only first event should be different */
			if (eventIndex == 0) {
				if ((pSensorEvent->data.u32 != 1) ||
				    (pSensorEvent->reserved1 != 0) ||
				    (pSensorEvent->reserved2 != 0) ||
				    (pSensorEvent->salt != 0) ||
				    (pSensorEvent->timestamp != 1) ||
				    (pSensorEvent->type != 1)) {
					result = failResult;
				}
			} else {
				/* All other events should be blank */
				if ((pSensorEvent->data.u32 != 0) ||
				    (pSensorEvent->reserved1 != 0) ||
				    (pSensorEvent->reserved2 != 0) ||
				    (pSensorEvent->salt != 0) ||
				    (pSensorEvent->timestamp != 0) ||
				    (pSensorEvent->type != 0)) {
					result = failResult;
				}
			}
		}
		if (result == 0) {
			if ((lczEventManagerData.eventIndex != 1) ||
			    (lczEventManagerData.eventSubIndex != 0) ||
			    (lczEventManagerData.lastEventTimestamp != 0)) {
				result = failResult;
			}
		}
	}
	/* Add a full event set to a blank file set and readback file and check indices */
	if (result == 0) {
		failResult++;

		lcz_event_manager_file_handler_unit_test_delete_all_files();
		lcz_event_manager_file_handler_rebuild_structure();
		lcz_event_manager_file_handler_get_indices();

		for (eventIndex = 0, eventType = 1;
		     eventIndex < TOTAL_NUMBER_EVENTS; eventIndex++) {
			/* Offset here for the timestamp to be unique */
			sensorEvent.data.u32 = eventIndex + 1;
			sensorEvent.type = eventType++;
			sensorEvent.timestamp = eventIndex + 1;

			lcz_event_manager_file_handler_add_event_private(
				&sensorEvent);
			/* Don't let event type go to 0 or it will appear blank*/
			if (eventType == 0) {
				eventType = 1;
			}
		}

		lcz_event_manager_file_handler_save_files();

		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);

		lcz_event_manager_file_handler_load_files();
		lcz_event_manager_file_handler_get_indices();

		for (eventIndex = 0, eventType = 1;
		     (eventIndex < TOTAL_NUMBER_EVENTS) && (result == 0);
		     eventIndex++) {
			pSensorEvent = lcz_event_manager_file_handler_get_event(
				eventIndex);

			/* All events should have been updated */
			if ((pSensorEvent->data.u32 != eventIndex + 1) ||
			    (pSensorEvent->reserved1 != 0) ||
			    (pSensorEvent->reserved2 != 0) ||
			    (pSensorEvent->salt != 0) ||
			    (pSensorEvent->timestamp != eventIndex + 1) ||
			    (pSensorEvent->type != eventType++)) {
				result = failResult;
			}
			/* Check event type hasn't gone to zero */
			if (eventType == 0) {
				eventType = 1;
			}
		}
		if (result == 0) {
			if ((lczEventManagerData.eventIndex != 0) ||
			    (lczEventManagerData.eventSubIndex != 0) ||
			    (lczEventManagerData.lastEventTimestamp != 0)) {
				result = failResult;
			}
		}
	}
	/* The following are for runtime conditions */
	if (result == 0) {
		failResult++;

		lcz_event_manager_file_handler_unit_test_delete_all_files();
		lcz_event_manager_file_handler_rebuild_structure();
		lcz_event_manager_file_handler_get_indices();

		for (eventIndex = 0, eventType = 1;
		     eventIndex < TOTAL_NUMBER_EVENTS; eventIndex++) {
			/* Offset here for the timestamp to be unique */
			sensorEvent.data.u32 = eventIndex + 1;
			sensorEvent.type = eventType++;
			sensorEvent.timestamp = eventIndex + 1;
			lcz_event_manager_file_handler_add_event_private(
				&sensorEvent);
			/* Don't let the timestamp go to 0xFFFF */
			/* This will cause issues for >UINT_MAX records */
			if (eventIndex == 0xFFFF) {
				eventIndex = 0;
			}
			/* Don't let event type go to zero */
			if (eventType == 0) {
				eventType = 1;
			}
		}

		lcz_event_manager_file_handler_save_files();

		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);

		lcz_event_manager_file_handler_load_files();
		lcz_event_manager_file_handler_get_indices();

		/* Now add 2 events with the same timestamp */
		sensorEvent.data.u32 = 0xFE;
		sensorEvent.timestamp = 0xFFFF;
		sensorEvent.type = 0xFE;

		lcz_event_manager_file_handler_add_event_private(&sensorEvent);

		sensorEvent.data.u32 = 0xFF;
		sensorEvent.timestamp = 0xFFFF;
		sensorEvent.type = 0xFF;

		lcz_event_manager_file_handler_add_event_private(&sensorEvent);

		/* And check the sub-index has been updated */
		for (eventIndex = 0, eventType = 3;
		     (eventIndex < TOTAL_NUMBER_EVENTS) && (result == 0);
		     eventIndex++) {
			pSensorEvent = lcz_event_manager_file_handler_get_event(
				eventIndex);

			if (eventIndex == 0) {
				if ((pSensorEvent->data.u32 != 0xFE) ||
				    (pSensorEvent->reserved1 != 0) ||
				    (pSensorEvent->reserved2 != 0) ||
				    (pSensorEvent->salt != 0) ||
				    (pSensorEvent->timestamp != 0xFFFF) ||
				    (pSensorEvent->type != 0xFE)) {
					result = failResult;
				}
			} else if (eventIndex == 1) {
				if ((pSensorEvent->data.u32 != 0xFF) ||
				    (pSensorEvent->reserved1 != 0) ||
				    (pSensorEvent->reserved2 != 0) ||
				    (pSensorEvent->salt != 1) ||
				    (pSensorEvent->timestamp != 0xFFFF) ||
				    (pSensorEvent->type != 0xFF)) {
					result = failResult;
				}
			} else {
				/* All other events should be at the previous value */
				if ((pSensorEvent->data.u32 !=
				     eventIndex + 1) ||
				    (pSensorEvent->reserved1 != 0) ||
				    (pSensorEvent->reserved2 != 0) ||
				    (pSensorEvent->salt != 0) ||
				    (pSensorEvent->timestamp !=
				     eventIndex + 1) ||
				    (pSensorEvent->type != eventType++)) {
					result = failResult;
				}
				if (eventType == 0) {
					eventType = 1;
				}
			}
		}
		/* We added two events so should be indexing the 3rd but */
		/* still expecting the next to arrive with the same timestamp */
		if (result == 0) {
			if ((lczEventManagerData.eventIndex != 2) ||
			    (lczEventManagerData.eventSubIndex != 2) ||
			    (lczEventManagerData.lastEventTimestamp !=
			     0xFFFF)) {
				result = failResult;
			}
		}
	}

	/* lcz_event_manager_file_handler_build_file */
	/* Create an output file with one entry */
	if (result == 0) {
		failResult++;
		log_file_status = LOG_FILE_STATUS_WAITING;
		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);

		lcz_event_manager_file_handler_get_indices();

		sensorEvent.data.u32 = 0xFE;
		sensorEvent.type = 0xFE;
		sensorEvent.timestamp = 0xFFFF;

		lcz_event_manager_file_handler_add_event_private(&sensorEvent);

		/* This will create the output file stats only */
		result = lcz_event_manager_file_handler_build_file(
			outputFileName, &outputFileSize, false);

		/* This is where the file lives */
		sprintf(fileName, "%s%s",
			CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
			LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME);

		/* Make sure the file was created OK */
		if (result != 0) {
			result = failResult;
		}
		/* Check the returned size */
		if (result == 0) {
			failResult++;
			if (outputFileSize != sizeof(SensorEvent_t)) {
				result = failResult;
			}
			/* Check output file name */
			if (strcmp(outputFileName, fileName)) {
				result = failResult;
			}
		}
	}

	/* Create an output file from a full event log */
	if (result == 0) {
		failResult++;
		log_file_status = LOG_FILE_STATUS_WAITING;
		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);

		lcz_event_manager_file_handler_get_indices();

		for (eventIndex = 0, eventType = 1;
		     eventIndex < TOTAL_NUMBER_EVENTS; eventIndex++) {
			sensorEvent.data.u32 = eventIndex;
			sensorEvent.type = eventType;
			sensorEvent.timestamp = eventIndex;

			lcz_event_manager_file_handler_add_event_private(
				&sensorEvent);

			/* Don't allow event type to go to RESERVED */
			eventType++;
			if (eventType == 0) {
				eventType = 1;
			}
		}
		/* This will create the output file stats only */
		result = lcz_event_manager_file_handler_build_file(
			outputFileName, &outputFileSize, false);

		/* This is where the file lives */
		fileName[0] = 0x0;
		sprintf(fileName, "%s%s",
			CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
			LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME);

		/* Check the file was created OK */
		if (result != 0) {
			result = failResult;
		}
		/* Check the returned file size */
		if (result == 0) {
			failResult++;
			if (outputFileSize !=
			    sizeof(SensorEvent_t) * TOTAL_NUMBER_EVENTS) {
				result = failResult;
			}
		}
		/* Check output file name */
		if (strcmp(outputFileName, fileName)) {
			result = failResult;
		}
	}

	/* lcz_event_manager_file_handler_background_build_file */

	/* Create an output file with one entry */
	if (result == 0) {
		failResult++;
		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);
		memset(eventManagerFileData.pIsDirty, false,
		       CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES);

		lcz_event_manager_file_handler_get_indices();

		sensorEvent.data.u32 = 0xFE;
		sensorEvent.type = 0xFE;
		sensorEvent.timestamp = 0xFFFF;

		lcz_event_manager_file_handler_add_event_private(&sensorEvent);

		/* This will create the output file */
		result =
			lcz_event_manager_file_handler_background_build_file(1);

		/* Make sure the file was created OK */
		if (result != 0) {
			result = failResult;
		}
		/* Check the actual file content */
		if (result == 0) {
			failResult++;
			count = 0;
			/* Get the file details */
			directoryEntry = fsu_find(
				CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
				LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME,
				&count, FS_DIR_ENTRY_FILE);
			/* Was it found? */
			if (directoryEntry == NULL) {
				/* No so leave here */
				result = failResult;
			} else {
				/* Is the size right? */
				if (directoryEntry->size !=
				    sizeof(SensorEvent_t)) {
					/* No so exit here */
					result = failResult;
				}
				/* Free up the directory entry either way */
				fsu_free_found(directoryEntry);
			}
			/* Check the output file content */
			if (result == 0) {
				/* This is what we expect to get back */
				memset(&sensorEvent, 0x0,
				       sizeof(SensorEvent_t));
				sensorEvent.data.u32 = 0xFE;
				sensorEvent.timestamp = 0xFFFF;
				sensorEvent.type = 0xFE;
				/* This is where the file lives */
				sprintf(fileName, "%s%s",
					CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
					LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME);
				/* Read back the next event details */
				if (fsu_read_abs(fileName, &sensorEventReadback,
						 sizeof(SensorEvent_t)) !=
				    sizeof(SensorEvent_t)) {
					/* Wrong data size returned */
					result = failResult;
				} else {
					if (memcmp(&sensorEvent,
						   &sensorEventReadback,
						   sizeof(SensorEvent_t))) {
						/* Wrong data read back */
						result = failResult;
					}
				}
			}
			/* Check that all events are blank */
			if (result == 0) {
				if (lcz_event_manager_file_handler_get_event_count() !=
				    0) {
					result = failResult;
				}
			}
			/* Check that the first file is dirty */
			if (result == 0) {
				if (eventManagerFileData.pIsDirty[0] != true) {
					result = failResult;
				}
			}
			/* And check all other files are not */
			if (result == 0) {
				for (fileIndex = 1;
				     (fileIndex <
				      CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES) &&
				     (result == 0);
				     fileIndex++) {
					if (eventManagerFileData
						    .pIsDirty[fileIndex] !=
					    false) {
						result = failResult;
					}
				}
			}
			/* Tidy up here if needed */
			fsu_delete_files(
				CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
				LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME);
		}
	}

	/* Create an output file from a full event log */
	if (result == 0) {
		failResult++;
		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);
		memset(eventManagerFileData.pIsDirty, false,
		       CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES);

		lcz_event_manager_file_handler_get_indices();

		for (eventIndex = 0, eventType = 1;
		     eventIndex < TOTAL_NUMBER_EVENTS; eventIndex++) {
			sensorEvent.data.u32 = eventIndex;
			sensorEvent.type = eventType;
			sensorEvent.timestamp = eventIndex;

			lcz_event_manager_file_handler_add_event_private(
				&sensorEvent);

			/* Don't allow event type to go to RESERVED */
			eventType++;
			if (eventType == 0) {
				eventType = 1;
			}
		}
		/* This will create an output file */
		result = lcz_event_manager_file_handler_background_build_file(
			TOTAL_NUMBER_EVENTS);

		/* Check the file was created OK */
		if (result != 0) {
			result = failResult;
		}
		/* Now check the actual file */
		if (result == 0) {
			failResult++;
			/* Check the output file size */
			count = 0;
			/* Get the file details */
			directoryEntry = fsu_find(
				CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
				LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME,
				&count, FS_DIR_ENTRY_FILE);
			/* Was it found? */
			if (directoryEntry == NULL) {
				/* No so leave here */
				result = failResult;
			} else {
				/* Is the size right? */
				if (directoryEntry->size !=
				    sizeof(SensorEvent_t) *
					    TOTAL_NUMBER_EVENTS) {
					/* No so exit here */
					result = failResult;
				}
				/* Free up the directory entry either way */
				fsu_free_found(directoryEntry);
			}
			/* Check the output file content */
			if (result == 0) {
				for (eventIndex = 0, eventType = 1;
				     (eventIndex < TOTAL_NUMBER_EVENTS) &&
				     (result == 0);
				     eventIndex++) {
					/* This is what we expect to get back */
					memset(&sensorEvent, 0x0,
					       sizeof(SensorEvent_t));
					sensorEvent.data.u32 = eventIndex;
					sensorEvent.timestamp = eventIndex;
					sensorEvent.type = eventType++;
					/* Don't allow event type to go to RESERVED */
					if (eventType == 0) {
						eventType = 1;
					}
					/* This is where the file lives */
					sprintf(fileName, "%s%s",
						CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
						LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME);
					/* Read back the next event details */
					struct fs_file_t f;
					fs_file_t_init(&f);
					result = fs_open(&f, fileName,
							 FS_O_READ);
					if (!result) {
						fs_seek(&f,
							sizeof(SensorEvent_t) *
								eventIndex,
							0);
						fs_read(&f,
							&sensorEventReadback,
							sizeof(SensorEvent_t));
					}
					fs_close(&f);
					if (result == 0) {
						if (memcmp(&sensorEvent,
							   &sensorEventReadback,
							   sizeof(SensorEvent_t))) {
							/* Wrong data read back */
							result = failResult;
						}
					}
				}
			}
			/* Check that all events are blank */
			if (result == 0) {
				if (lcz_event_manager_file_handler_get_event_count() !=
				    0) {
					result = failResult;
				}
			}
			/* Check that all files are dirty */
			if (result == 0) {
				for (fileIndex = 0;
				     (fileIndex <
				      CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES) &&
				     (result == 0);
				     fileIndex++) {
					if (eventManagerFileData
						    .pIsDirty[fileIndex] !=
					    true) {
						result = failResult;
					}
				}
			}
			/* Tidy up here if needed */
			fsu_delete_files(
				CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
				LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME);
		}
	}

	/* lcz_event_manager_file_handler_get_subindexed_event */
	/* Invalid event index */
	if (result == 0) {
		failResult++;

		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);

		pSensorEvent =
			lcz_event_manager_file_handler_get_subindexed_event(
				TOTAL_NUMBER_EVENTS, 0, 1);

		if (pSensorEvent != NULL) {
			result = failResult;
		}
	}
	/* One event at the start of the event log */
	if (result == 0) {
		failResult++;

		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);

		pSensorEvent =
			lcz_event_manager_file_handler_get_subindexed_event(
				0, 0, 1);

		if (pSensorEvent !=
		    lcz_event_manager_file_handler_get_event(0)) {
			result = failResult;
		}
	}
	/* One event at the end of the log */
	if (result == 0) {
		failResult++;

		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);

		pSensorEvent =
			lcz_event_manager_file_handler_get_subindexed_event(
				TOTAL_NUMBER_EVENTS - 1, 0, 1);

		if (pSensorEvent != lcz_event_manager_file_handler_get_event(
					    TOTAL_NUMBER_EVENTS - 1)) {
			result = failResult;
		}
	}
	/* One event at the end of the log and another at the start */
	/* Index event at end of log first                          */
	if (result == 0) {
		failResult++;

		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);

		pSensorEvent =
			lcz_event_manager_file_handler_get_subindexed_event(
				TOTAL_NUMBER_EVENTS - 1, 0, 2);

		if (pSensorEvent != lcz_event_manager_file_handler_get_event(
					    TOTAL_NUMBER_EVENTS - 1)) {
			result = failResult;
		}
	}
	/* Then the event at the start of the log */
	if (result == 0) {
		failResult++;

		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);

		lcz_event_manager_file_handler_get_event(0)->salt = 1;

		pSensorEvent =
			lcz_event_manager_file_handler_get_subindexed_event(
				TOTAL_NUMBER_EVENTS - 1, 1, 2);

		if (pSensorEvent !=
		    lcz_event_manager_file_handler_get_event(0)) {
			result = failResult;
		}
	}

	/* lcz_event_manager_file_handler_get_indexed_event_at_timestamp */
	/* One event at the requested timestamp at the start of the log */
	if (result == 0) {
		failResult++;

		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);

		pSensorEvent = lcz_event_manager_file_handler_get_event(0);
		pSensorEvent->timestamp = 0xFFA;
		pSensorEvent->type = 0x1;

		pSensorEvent =
			lcz_event_manager_file_handler_get_indexed_event_at_timestamp(
				0xFFA, 0, &eventCount);

		if (pSensorEvent !=
		    lcz_event_manager_file_handler_get_event(0)) {
			result = failResult;
		}

		/* Count correct? */
		if (result == 0) {
			if (eventCount != 1) {
				result = failResult;
			}
		}
	}
	/* Two events at the start of the log */
	if (result == 0) {
		failResult++;

		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);

		/* This is the event at index 0 */
		pSensorEvent = lcz_event_manager_file_handler_get_event(0);
		pSensorEvent->timestamp = 0xFFA;
		pSensorEvent->type = 0x1;
		/* This is the event at index 1 */
		pSensorEvent = lcz_event_manager_file_handler_get_event(1);
		pSensorEvent->timestamp = 0xFFA;
		pSensorEvent->type = 0x2;
		pSensorEvent->salt = 0x1;

		pSensorEvent =
			lcz_event_manager_file_handler_get_indexed_event_at_timestamp(
				0xFFA, 0, &eventCount);

		/* First event OK? */
		if (pSensorEvent !=
		    lcz_event_manager_file_handler_get_event(0)) {
			result = failResult;
		}
		/* Count correct? */
		if (result == 0) {
			if (eventCount != 2) {
				result = failResult;
			}
		}
		/* Second event OK ? */
		if (result == 0) {
			pSensorEvent =
				lcz_event_manager_file_handler_get_indexed_event_at_timestamp(
					0xFFA, 1, &eventCount);
			if (pSensorEvent !=
			    lcz_event_manager_file_handler_get_event(1)) {
				result = failResult;
			}
		}
	}
	/* Events at end and start of log */
	if (result == 0) {
		failResult++;

		memset(eventManagerFileData.pFileData, SENSOR_EVENT_RESERVED,
		       TOTAL_FILE_SIZE_BYTES);

		pSensorEvent = lcz_event_manager_file_handler_get_event(
			TOTAL_NUMBER_EVENTS - 1);
		pSensorEvent->timestamp = 0xFFA;
		pSensorEvent->type = 0x1;
		pSensorEvent = lcz_event_manager_file_handler_get_event(0);
		pSensorEvent->timestamp = 0xFFA;
		pSensorEvent->type = 0x2;
		pSensorEvent->salt = 0x1;

		pSensorEvent =
			lcz_event_manager_file_handler_get_indexed_event_at_timestamp(
				0xFFA, 0, &eventCount);

		/* First event OK? */
		if (pSensorEvent != lcz_event_manager_file_handler_get_event(
					    TOTAL_NUMBER_EVENTS - 1)) {
			result = failResult;
		}
		/* Count correct? */
		if (result == 0) {
			if (eventCount != 2) {
				result = failResult;
			}
		}
		/* Second event OK ? */
		if (result == 0) {
			pSensorEvent =
				lcz_event_manager_file_handler_get_indexed_event_at_timestamp(
					0xFFA, 1, &eventCount);
			if (pSensorEvent !=
			    lcz_event_manager_file_handler_get_event(0)) {
				result = failResult;
			}
		}
	}

	/* lcz_event_manager_file_handler_background_build_dummy_event */
	/* Bool type */
	if (result == 0) {
		failResult++;

		/* Build the properties used by the dummy event builder */
		dummy_log_properties.start_time_stamp = 0;
		dummy_log_properties.update_rate = 1;
		dummy_log_properties.event_type = 0;
		dummy_log_properties.event_count = 1;
		dummy_log_properties.event_data_type = DUMMY_LOG_DATA_TYPE_BOOL;

		/* Build it */
		lcz_event_manager_file_handler_background_build_dummy_event(
			&sensorEvent, 0, &dummy_log_properties, 0);

		/* Check it */
		if ((sensorEvent.timestamp != 0) ||
		    (sensorEvent.data.u16 != 0) || (sensorEvent.type != 0) ||
		    (sensorEvent.salt != 0)) {
			result = failResult;
		}
	}
	/* U32 type */
	if (result == 0) {
		failResult++;

		/* Build the properties used by the dummy event builder */
		dummy_log_properties.start_time_stamp = 1000;
		dummy_log_properties.update_rate = 10;
		dummy_log_properties.event_type = 1;
		dummy_log_properties.event_count = 1;
		dummy_log_properties.event_data_type = DUMMY_LOG_DATA_TYPE_U32;

		/* Build it */
		lcz_event_manager_file_handler_background_build_dummy_event(
			&sensorEvent, 1000, &dummy_log_properties, 100);

		/* Check it */
		if ((sensorEvent.timestamp != 1000) ||
		    (sensorEvent.data.u32 != 100) || (sensorEvent.type != 1) ||
		    (sensorEvent.salt != 0)) {
			result = failResult;
		}
	}
	/* Float type */
	if (result == 0) {
		failResult++;

		/* Build the properties used by the dummy event builder */
		dummy_log_properties.start_time_stamp = 5000;
		dummy_log_properties.update_rate = 10;
		dummy_log_properties.event_type = 10;
		dummy_log_properties.event_count = 1;
		dummy_log_properties.event_data_type =
			DUMMY_LOG_DATA_TYPE_FLOAT;

		/* Build it */
		lcz_event_manager_file_handler_background_build_dummy_event(
			&sensorEvent, 5000, &dummy_log_properties, 500);

		/* Check it */
		if ((sensorEvent.timestamp != 5000) ||
		    (sensorEvent.data.f != 500.0f) ||
		    (sensorEvent.type != 10) || (sensorEvent.salt != 0)) {
			result = failResult;
		}
	}
	/* lcz_event_manager_file_handler_background_build_dummy_file */
	/* 10 Boolean events */
	if (result == 0) {
		failResult++;

		/* Build the properties used by the dummy event builder */
		dummy_log_properties.start_time_stamp = 0;
		dummy_log_properties.update_rate = 1;
		dummy_log_properties.event_type = 0;
		dummy_log_properties.event_count = 10;
		dummy_log_properties.event_data_type = DUMMY_LOG_DATA_TYPE_BOOL;

		/* Build it */
		result =
			lcz_event_manager_file_handler_background_build_dummy_file(
				&dummy_log_properties);

		/* This is where the file lives */
		sprintf(fileName, "%s%s",
			CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
			LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME);

		/* Check return code */
		if (result != 0) {
			result = failResult;
		}
		/* Check file created OK */
		if (result == 0) {
			if (fsu_get_file_size(
				    CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
				    LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME) !=
			    10 * sizeof(SensorEvent_t)) {
				result = failResult;
			}
		}
		/* Check file content */
		if (result == 0) {
			timestamp = 0;
			data = 0;
			for (eventIndex = 0; (eventIndex < 10) && (result == 0);
			     eventIndex++) {
				/* Read out the next event */
				struct fs_file_t f;
				fs_file_t_init(&f);
				result = fs_open(&f, fileName, FS_O_READ);
				if (!result) {
					fs_seek(&f,
						sizeof(SensorEvent_t) *
							eventIndex,
						0);
					fs_read(&f, &sensorEventReadback,
						sizeof(SensorEvent_t));
				}
				fs_close(&f);
				/* Check the values */
				if ((sensorEventReadback.data.u16 != data) ||
				    (sensorEventReadback.salt != 0) ||
				    (sensorEventReadback.timestamp !=
				     timestamp) ||
				    (sensorEventReadback.type != 0)) {
					result = failResult;
				}
				/* Update expected values if checked OK */
				if (result == 0) {
					timestamp++;
					data = !data;
				}
			}
		}
	}
	/* 100 Float events */
	if (result == 0) {
		failResult++;

		/* Build the properties used by the dummy event builder */
		dummy_log_properties.start_time_stamp = 250;
		dummy_log_properties.update_rate = 10;
		dummy_log_properties.event_type = 1;
		dummy_log_properties.event_count = 100;
		dummy_log_properties.event_data_type =
			DUMMY_LOG_DATA_TYPE_FLOAT;

		/* Build it */
		result =
			lcz_event_manager_file_handler_background_build_dummy_file(
				&dummy_log_properties);

		/* This is where the file lives */
		sprintf(fileName, "%s%s",
			CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
			LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME);

		/* Check return code */
		if (result != 0) {
			result = failResult;
		}
		/* Check file created OK */
		if (result == 0) {
			if (fsu_get_file_size(
				    CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
				    LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME) !=
			    100 * sizeof(SensorEvent_t)) {
				result = failResult;
			}
		}
		/* Check file content */
		if (result == 0) {
			timestamp = 250;
			data = 0;
			for (eventIndex = 0;
			     (eventIndex < 100) && (result == 0);
			     eventIndex++) {
				/* Read out the next event */
				struct fs_file_t f;
				fs_file_t_init(&f);
				result = fs_open(&f, fileName, FS_O_READ);
				if (!result) {
					fs_seek(&f,
						sizeof(SensorEvent_t) *
							eventIndex,
						0);
					fs_read(&f, &sensorEventReadback,
						sizeof(SensorEvent_t));
				}
				fs_close(&f);
				/* Check the values */
				if ((sensorEventReadback.data.f !=
				     ((float)(data))) ||
				    (sensorEventReadback.salt != 0) ||
				    (sensorEventReadback.timestamp !=
				     timestamp) ||
				    (sensorEventReadback.type != 1)) {
					result = failResult;
				}
				/* Update expected values if checked OK */
				if (result == 0) {
					timestamp += 10;
					data++;
				}
			}
		}
	}
	/* 1000 U32 events */
	if (result == 0) {
		failResult++;

		/* Build the properties used by the dummy event builder */
		dummy_log_properties.start_time_stamp = 500;
		dummy_log_properties.update_rate = 5;
		dummy_log_properties.event_type = 2;
		dummy_log_properties.event_count = 1000;
		dummy_log_properties.event_data_type = DUMMY_LOG_DATA_TYPE_U32;

		/* Build it */
		result =
			lcz_event_manager_file_handler_background_build_dummy_file(
				&dummy_log_properties);

		/* This is where the file lives */
		sprintf(fileName, "%s%s",
			CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
			LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME);

		/* Check return code */
		if (result != 0) {
			result = failResult;
		}
		/* Check file created OK */
		if (result == 0) {
			if (fsu_get_file_size(
				    CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
				    LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME) !=
			    1000 * sizeof(SensorEvent_t)) {
				result = failResult;
			}
		}
		/* Check file content */
		if (result == 0) {
			timestamp = 500;
			data = 0;
			for (eventIndex = 0;
			     (eventIndex < 1000) && (result == 0);
			     eventIndex++) {
				/* Read out the next event */
				struct fs_file_t f;
				fs_file_t_init(&f);
				result = fs_open(&f, fileName, FS_O_READ);
				if (!result) {
					fs_seek(&f,
						sizeof(SensorEvent_t) *
							eventIndex,
						0);
					fs_read(&f, &sensorEventReadback,
						sizeof(SensorEvent_t));
				}
				fs_close(&f);
				/* Check the values */
				if ((sensorEventReadback.data.u32 != data) ||
				    (sensorEventReadback.salt != 0) ||
				    (sensorEventReadback.timestamp !=
				     timestamp) ||
				    (sensorEventReadback.type != 2)) {
					result = failResult;
				}
				/* Update expected values if checked OK */
				if (result == 0) {
					timestamp += 5;
					data++;
				}
			}
		}
	}
	/* 100 U32 events with duplicate timestamp to increment salt */
	if (result == 0) {
		failResult++;

		/* Build the properties used by the dummy event builder */
		dummy_log_properties.start_time_stamp = 1000;
		dummy_log_properties.update_rate = 0;
		dummy_log_properties.event_type = 3;
		dummy_log_properties.event_count = 100;
		dummy_log_properties.event_data_type = DUMMY_LOG_DATA_TYPE_U32;

		/* Build it */
		result =
			lcz_event_manager_file_handler_background_build_dummy_file(
				&dummy_log_properties);

		/* This is where the file lives */
		sprintf(fileName, "%s%s",
			CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
			LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME);

		/* Check return code */
		if (result != 0) {
			result = failResult;
		}
		/* Check file created OK */
		if (result == 0) {
			if (fsu_get_file_size(
				    CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PUBLIC_DIRECTORY,
				    LCZ_EVENT_MANAGER_FILE_HANDLER_OUTPUT_FILE_NAME) !=
			    100 * sizeof(SensorEvent_t)) {
				result = failResult;
			}
		}
		/* Check file content */
		if (result == 0) {
			timestamp = 1000;
			data = 0;
			salt = 0;
			for (eventIndex = 0;
			     (eventIndex < 100) && (result == 0);
			     eventIndex++) {
				/* Read out the next event */
				struct fs_file_t f;
				fs_file_t_init(&f);
				result = fs_open(&f, fileName, FS_O_READ);
				if (!result) {
					fs_seek(&f,
						sizeof(SensorEvent_t) *
							eventIndex,
						0);
					fs_read(&f, &sensorEventReadback,
						sizeof(SensorEvent_t));
				}
				fs_close(&f);
				/* Check the values */
				if ((sensorEventReadback.data.u32 != data) ||
				    (sensorEventReadback.salt != salt) ||
				    (sensorEventReadback.timestamp !=
				     timestamp) ||
				    (sensorEventReadback.type != 3)) {
					result = failResult;
				}
				/* Update expected values if checked OK */
				if (result == 0) {
					salt++;
					data++;
				}
			}
		}
	}
	return (result);
}

static void lcz_event_manager_file_handler_unit_test_delete_all_files(void)
{
	uint16_t fileIndex;
	uint8_t fileName[LCZ_EVENT_MANAGER_FILENAME_SIZE];
	int result = 0;
	struct fs_dirent *pDirectoryEntry;
	size_t fileCount;

	for (fileIndex = 0;
	     (fileIndex < CONFIG_LCZ_EVENT_MANAGER_NUMBER_OF_FILES) &&
	     (result == 0);
	     fileIndex++) {
		/* Reset the filecount each time we process a file */
		fileCount = 0;

		/* Build the next file name */
		sprintf(fileName, "%s%d",
			LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_FILE_NAME,
			fileIndex);

		/* First get the details of the file */
		pDirectoryEntry = fsu_find(
			CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_DIRECTORY,
			fileName, &fileCount, FS_DIR_ENTRY_FILE);

		/* Is the file available? */
		if (pDirectoryEntry != NULL) {
			/* Yes, so delete it */
			fsu_delete_files(
				CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_DIRECTORY,
				fileName);
			/* And free up the directory entry */
			fsu_free_found(pDirectoryEntry);
		}
	}
}

static int
lcz_event_manager_file_handler_unit_test_create_file(uint8_t fileIndex,
						     uint16_t fileSize)
{
	uint8_t fileName[LCZ_EVENT_MANAGER_FILENAME_SIZE];
	struct fs_dirent *pDirectoryEntry;
	size_t fileCount;
	int result = 0;

	/* Reset the filecount each time we process a file */
	fileCount = 0;

	/* Build the next file name */
	sprintf(fileName, "%s%d",
		LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_FILE_NAME, fileIndex);

	/* First get the details of the file */
	pDirectoryEntry = fsu_find(
		CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_DIRECTORY,
		fileName, &fileCount, FS_DIR_ENTRY_FILE);

	/* Is the file available? */
	if (pDirectoryEntry != NULL) {
		/* Yes, so delete it */
		fsu_delete_files(
			CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_DIRECTORY,
			fileName);
		/* And free up the directory entry */
		fsu_free_found(pDirectoryEntry);
	}

	/* Now create the file */
	sprintf(fileName, "%s%s%d",
		CONFIG_LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_DIRECTORY,
		LCZ_EVENT_MANAGER_FILE_HANDLER_PRIVATE_FILE_NAME, fileIndex);

	fsu_write_abs(fileName, eventManagerFileData.pFileData[fileIndex],
		      fileSize);

	return (result);
}
/* End of unit test code */
#endif
