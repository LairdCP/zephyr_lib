/**
 * @file test_lcz_qrtc_errno.c
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
#include <errno.h>
#include "test_lcz_qrtc.h"
#include "lcz_qrtc.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
typedef enum {
        LCZ_QRTC_SLEEP_TIME_5S = 5
} lcz_wdt_sleep_time_t;

typedef enum {
        LCZ_QRTC_MONTH_JANUARY = 0,
        LCZ_QRTC_MONTH_FEBRUARY,
        LCZ_QRTC_MONTH_MARCH,
        LCZ_QRTC_MONTH_APRIL,
        LCZ_QRTC_MONTH_MAY,
        LCZ_QRTC_MONTH_JUNE,
        LCZ_QRTC_MONTH_JULY,
        LCZ_QRTC_MONTH_AUGUST,
        LCZ_QRTC_MONTH_SEPTEMBER,
        LCZ_QRTC_MONTH_OCTOBER,
        LCZ_QRTC_MONTH_NOVEMBER,
        LCZ_QRTC_MONTH_DECEMBER,
} lcz_qrtc_month_t;

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static void update_tm_year(struct tm *time_field);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void test_lcz_qrtc_errno(void)
{
	/* LCZ QRTC Test 1:
	 *   Check errno is correctly set when invalid input values are
	 *   supplied and not set when valid values are supplied to QRTC
	 *   functions
	 */
        struct tm time_field;
	uint32_t qrtc_actual;
        int32_t qrtc_offset;

	/* Sleep for a short period of time to increase uptime count */
	k_sleep(K_SECONDS(LCZ_QRTC_SLEEP_TIME_5S));

	/* Set QRTC to less than uptime (1 < 5) - expected fail */
	errno = 0;
	qrtc_actual = lcz_qrtc_set_epoch(1);
	zassert_equal(errno, -EINVAL, "errno = -EINVAL mismatch");
	zassert_not_equal(qrtc_actual, 1, "QRTC should not be 1");
	zassert_false(lcz_qrtc_epoch_was_set(), "QRTC should not be set");

	/* Set QRTC to less than uptime (2 < 5) - expected fail */
	errno = 0;
	qrtc_actual = lcz_qrtc_set_epoch(2);
	zassert_equal(errno, -EINVAL, "errno = -EINVAL mismatch");
	zassert_not_equal(qrtc_actual, 2, "QRTC should not be 2");
	zassert_false(lcz_qrtc_epoch_was_set(), "QRTC should not be set");

	/* Set QRTC to less than uptime (3 < 5) - expected fail */
	errno = 0;
	qrtc_actual = lcz_qrtc_set_epoch(3);
	zassert_equal(errno, -EINVAL, "errno = -EINVAL mismatch");
	zassert_not_equal(qrtc_actual, 3, "QRTC should not be 3");
	zassert_false(lcz_qrtc_epoch_was_set(), "QRTC should not be set");

	/* Set QRTC to greater than uptime (90000 > 5) */
	errno = 0;
	qrtc_actual = lcz_qrtc_set_epoch(90000);
	zassert_ok(errno, "errno = 0 mismatch");
	zassert_equal(qrtc_actual, 90000, "QRTC should be set");
	zassert_true(lcz_qrtc_epoch_was_set(), "QRTC should be set");

	/* Set QRTC with too low an offset - expected fail */
        time_field.tm_sec = 13;
        time_field.tm_min = 26;
        time_field.tm_hour = 3;
        time_field.tm_mday = 5;
        time_field.tm_mon = LCZ_QRTC_MONTH_JANUARY;
        time_field.tm_year = 2000;
        qrtc_offset = -86410;
        update_tm_year(&time_field);
	errno = 0;
        qrtc_actual = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	zassert_equal(errno, -EINVAL, "errno = -EINVAL mismatch");
	zassert_equal(qrtc_actual, 90000, "QRTC should be set");
	zassert_true(lcz_qrtc_epoch_was_set(), "QRTC should be set");

	/* Set QRTC with too high an offset - expected fail */
        time_field.tm_sec = 13;
        time_field.tm_min = 26;
        time_field.tm_hour = 3;
        time_field.tm_mday = 5;
        time_field.tm_mon = LCZ_QRTC_MONTH_JANUARY;
        time_field.tm_year = 2000;
        qrtc_offset = 86401;
        update_tm_year(&time_field);
	errno = 0;
        qrtc_actual = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	zassert_equal(errno, -EINVAL, "errno = -EINVAL mismatch");
	zassert_equal(qrtc_actual, 90000, "QRTC should be set");
	zassert_true(lcz_qrtc_epoch_was_set(), "QRTC should be set");

	/* Set QRTC with epoch < offset (3661 < 83000) - expected fail */
        time_field.tm_sec = 1;
        time_field.tm_min = 1;
        time_field.tm_hour = 1;
        time_field.tm_mday = 1;
        time_field.tm_mon = LCZ_QRTC_MONTH_JANUARY;
        time_field.tm_year = 1970;
        qrtc_offset = 83000;
        update_tm_year(&time_field);
	errno = 0;
        qrtc_actual = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	zassert_equal(errno, -EINVAL, "errno = -EINVAL mismatch");
	zassert_equal(qrtc_actual, 90000, "QRTC should be set");
	zassert_true(lcz_qrtc_epoch_was_set(), "QRTC should be set");

	/* Set QRTC with pre-epoch (1970) value - expected fail */
        time_field.tm_sec = 1;
        time_field.tm_min = 1;
        time_field.tm_hour = 1;
        time_field.tm_mday = 1;
        time_field.tm_mon = LCZ_QRTC_MONTH_JANUARY;
        time_field.tm_year = 1960;
        qrtc_offset = 0;
        update_tm_year(&time_field);
	errno = 0;
        qrtc_actual = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	zassert_equal(errno, -EINVAL, "errno = -EINVAL mismatch");
	zassert_equal(qrtc_actual, 90000, "QRTC should be set");
	zassert_true(lcz_qrtc_epoch_was_set(), "QRTC should be set");
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static void update_tm_year(struct tm *time_field)
{
        time_field->tm_year -= 1900;
}
