/**
 * @file test_lcz_qrtc_minimum_epoch.c
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
typedef enum {
	LCZ_QRTC_SLEEP_TIME_3S = 3
} lcz_wdt_sleep_time_t;

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void test_lcz_qrtc_minimum_epoch(void)
{
	/* LCZ QRTC Test 1:
	 *   Try setting epoch with a minimum epoch Kconfig value enabled
	 */
	uint32_t qrtc_set, qrtc_get;

	zassert_false(lcz_qrtc_epoch_was_set(),
		      "QRTC is set when it should not be");

	qrtc_set = 0;
	qrtc_get = lcz_qrtc_set_epoch(qrtc_set);
	zassert_equal(0, qrtc_get, "QRTC get/set value mismatch");
	zassert_false(lcz_qrtc_epoch_was_set(),
		      "QRTC is set when it should not be");

	qrtc_set = 1500;
	qrtc_get = lcz_qrtc_set_epoch(qrtc_set);
	zassert_not_equal(qrtc_set, qrtc_get,
			  "QRTC set value should have failed");
	zassert_equal(0, qrtc_get, "QRTC get/set value mismatch");
	zassert_false(lcz_qrtc_epoch_was_set(),
		      "QRTC is set when it should not be");

	qrtc_set = 1500000;
	qrtc_get = lcz_qrtc_set_epoch(qrtc_set);
	zassert_not_equal(qrtc_set, qrtc_get,
			  "QRTC set value should have failed");
	zassert_equal(0, qrtc_get, "QRTC get/set value mismatch");
	zassert_false(lcz_qrtc_epoch_was_set(),
		      "QRTC is set when it should not be");

	qrtc_set = 1500000000;
	qrtc_get = lcz_qrtc_set_epoch(qrtc_set);
	zassert_not_equal(qrtc_set, qrtc_get,
			  "QRTC set value should have failed");
	zassert_equal(0, qrtc_get, "QRTC get/set value mismatch");
	zassert_false(lcz_qrtc_epoch_was_set(),
		      "QRTC is set when it should not be");

	qrtc_set = 1600000000;
	qrtc_get = lcz_qrtc_set_epoch(qrtc_set);
	zassert_not_equal(qrtc_set, qrtc_get,
			  "QRTC set value should have failed");
	zassert_equal(0, qrtc_get, "QRTC get/set value mismatch");
	zassert_false(lcz_qrtc_epoch_was_set(),
		      "QRTC is set when it should not be");

	qrtc_set = 1637012344;
	qrtc_get = lcz_qrtc_set_epoch(qrtc_set);
	zassert_not_equal(qrtc_set, qrtc_get,
			  "QRTC set value should have failed");
	zassert_equal(0, qrtc_get, "QRTC get/set value mismatch");
	zassert_false(lcz_qrtc_epoch_was_set(),
		      "QRTC is set when it should not be");

	qrtc_set = 1637012345;
	qrtc_get = lcz_qrtc_set_epoch(qrtc_set);
	zassert_equal(qrtc_set, qrtc_get,
			  "QRTC set value should have succeeded");
	zassert_not_equal(0, qrtc_get, "QRTC get/set value mismatch");
	zassert_true(lcz_qrtc_epoch_was_set(),
		     "QRTC is not set when it should be");

	qrtc_set = 1600000000;
	qrtc_get = lcz_qrtc_set_epoch(qrtc_set);
	zassert_not_equal(qrtc_set, qrtc_get,
			  "QRTC set value should have failed");
	zassert_not_equal(0, qrtc_get, "QRTC get/set value mismatch");
	zassert_equal(1637012345, qrtc_get, "QRTC get/set value mismatch");
	zassert_true(lcz_qrtc_epoch_was_set(),
		     "QRTC is not set when it should be");

	qrtc_set = 1637012345 + LCZ_QRTC_SLEEP_TIME_3S;
	k_sleep(K_SECONDS(LCZ_QRTC_SLEEP_TIME_3S));
	qrtc_get = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_set, qrtc_get, "QRTC get/set value mismatch");
}
