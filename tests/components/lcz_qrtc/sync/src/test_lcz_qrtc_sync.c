/**
 * @file test_lcz_qrtc_sync.c
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr.h>
#include <ztest.h>
#include <time.h>
#include "test_lcz_qrtc.h"
#include "lcz_qrtc.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define QRTC_SEMAPHORE_WAIT_TIME_S (CONFIG_LCZ_QRTC_SYNC_INTERVAL_SECONDS + 1)

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static struct k_sem time_sync_sem;
static uint8_t sync_runs = 0;
static uint32_t times[] = {
	4000,
	108000,
	9000,
	1795140273,
	1040302010
};
static const uint8_t sync_items = sizeof(times) / sizeof(times[0]);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void lcz_qrtc_test_setup(void)
{
	k_sem_init(&time_sync_sem, 0, 1);
}

void lcz_qrtc_sync_handler(void)
{
	uint32_t qrtc_set;

	if (sync_runs < sync_items) {
		qrtc_set = lcz_qrtc_set_epoch(times[sync_runs]);
		zassert_equal(qrtc_set, times[sync_runs],
			      "QRTC set value mismatch");
	}

	++sync_runs;

	k_sem_give(&time_sync_sem);
}

void test_lcz_qrtc_sync(void)
{
	/* LCZ QRTC Test 1:
	 *   Check QRTC sync handler is called and updates time
	 */
	uint8_t i = 0;
	uint32_t last_time;
	uint32_t extra_time;
	int rc;

	while (i < 10) {
		TC_PRINT("Loop %d\n", (i + 1));
		rc = k_sem_take(&time_sync_sem,
				K_SECONDS(QRTC_SEMAPHORE_WAIT_TIME_S));
		zassert_ok(rc, "Sync QRTC semaphore timeout");

		if (i < sync_items) {
			/* Check that the current epoch is within +/-1 seconds
			 * of the set value
			 */
			zassert_within(lcz_qrtc_get_epoch(), times[i], 1,
				       "Epoch not +/-1 seconds of previous set value");
			last_time = lcz_qrtc_get_epoch();
		} else {
			/* Epoch should be counting up from the last array
			 * entry, which is 1040302010
			 */
			extra_time = (i + 1 - sync_items) *
				     CONFIG_LCZ_QRTC_SYNC_INTERVAL_SECONDS;
			zassert_equal(lcz_qrtc_get_epoch(),
				      (last_time + extra_time),
				      "QRTC get value mismatch");
		}

		++i;
	}

	zassert_equal(i, sync_runs, "QRTC sync/read loop mismatch");
}
