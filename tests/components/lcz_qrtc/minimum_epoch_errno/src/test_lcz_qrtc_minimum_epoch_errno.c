/**
 * @file test_lcz_qrtc_minimum_epoch_errno.c
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
void test_lcz_qrtc_minimum_epoch_errno(void)
{
	/* LCZ QRTC Test 1:
	 *   Check errno is correctly set when invalid input values are
	 *   supplied and not set when valid values are supplied to QRTC
	 *   functions with the minimum epoch option enabled
	 */
        struct tm time_field;
	uint32_t qrtc_actual;
        int32_t qrtc_offset;

	/* Sleep for a short period of time to increase uptime count */
	k_sleep(K_SECONDS(LCZ_QRTC_SLEEP_TIME_5S));

	/* Set QRTC to less than minimum epoch - expected fail */
	errno = 0;
	qrtc_actual = lcz_qrtc_set_epoch(1637011345);
	zassert_equal(errno, -EINVAL, "errno = -EINVAL mismatch");
	zassert_not_equal(qrtc_actual, 1, "QRTC should not be 1");
	zassert_false(lcz_qrtc_epoch_was_set(), "QRTC should not be set");

	/* Set QRTC to less than minimum epoch - expected fail */
	errno = 0;
	qrtc_actual = lcz_qrtc_set_epoch(1637012344);
	zassert_equal(errno, -EINVAL, "errno = -EINVAL mismatch");
	zassert_not_equal(qrtc_actual, 2, "QRTC should not be 2");
	zassert_false(lcz_qrtc_epoch_was_set(), "QRTC should not be set");

	/* Set QRTC to greater than minimum epoch */
	errno = 0;
	qrtc_actual = lcz_qrtc_set_epoch(1637091345);
	zassert_ok(errno, "errno = 0 mismatch");
	zassert_equal(qrtc_actual, 1637091345, "QRTC should be set");
	zassert_true(lcz_qrtc_epoch_was_set(), "QRTC should be set");

	/* Set QRTC to less than minimum epoch - expected fail */
        time_field.tm_sec = 16;
        time_field.tm_min = 5;
        time_field.tm_hour = 4;
        time_field.tm_mday = 3;
        time_field.tm_mon = LCZ_QRTC_MONTH_OCTOBER;
        time_field.tm_year = 2021;
        qrtc_offset = 0;
        update_tm_year(&time_field);
	errno = 0;
        qrtc_actual = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	zassert_equal(errno, -EINVAL, "errno = -EINVAL mismatch");
	zassert_equal(qrtc_actual, 1637091345, "QRTC should be set");
	zassert_true(lcz_qrtc_epoch_was_set(), "QRTC should be set");
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static void update_tm_year(struct tm *time_field)
{
        time_field->tm_year -= 1900;
}
