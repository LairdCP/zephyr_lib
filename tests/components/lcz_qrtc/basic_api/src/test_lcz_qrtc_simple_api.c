/**
 * @file test_lcz_qrtc_simple_api.c
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
	LCZ_QRTC_SLEEP_TIME_1S = 1,
	LCZ_QRTC_SLEEP_TIME_3S = 3,
	LCZ_QRTC_SLEEP_TIME_7S = 7
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
void test_lcz_qrtc_is_not_set(void)
{
	/* LCZ QRTC Test 1:
	 *   Check QRTC is not set at start-up
	 */
	zassert_false(lcz_qrtc_epoch_was_set(),
		      "QRTC is set when it should not be");
}

void test_lcz_qrtc_set_get(void)
{
	/* LCZ QRTC Test 2:
	 *   Check set and get epoch functions with various input values
	 */
	uint32_t qrtc_set, qrtc_get;

	qrtc_set = 14;
	qrtc_get = lcz_qrtc_set_epoch(qrtc_set);
	zassert_equal(qrtc_set, qrtc_get, "QRTC set value mismatch");
	qrtc_get = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_set, qrtc_get, "QRTC get/set value mismatch");

	qrtc_set = 1000;
	qrtc_get = lcz_qrtc_set_epoch(qrtc_set);
	zassert_equal(qrtc_set, qrtc_get, "QRTC set value mismatch");
	qrtc_get = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_set, qrtc_get, "QRTC get/set value mismatch");

	qrtc_set = 9;
	qrtc_get = lcz_qrtc_set_epoch(qrtc_set);
	zassert_equal(qrtc_set, qrtc_get, "QRTC set value mismatch");
	qrtc_get = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_set, qrtc_get, "QRTC get/set value mismatch");

	qrtc_set = 1638268019;
	qrtc_get = lcz_qrtc_set_epoch(qrtc_set);
	zassert_equal(qrtc_set, qrtc_get, "QRTC set value mismatch");
	qrtc_get = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_set, qrtc_get, "QRTC get/set value mismatch");

	qrtc_set = 4294967021;
	qrtc_get = lcz_qrtc_set_epoch(qrtc_set);
	zassert_equal(qrtc_set, qrtc_get, "QRTC set value mismatch");
	qrtc_get = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_set, qrtc_get, "QRTC get/set value mismatch");

	qrtc_set = 0;
	qrtc_get = lcz_qrtc_set_epoch(qrtc_set);
	zassert_equal(qrtc_set, qrtc_get, "QRTC set value mismatch");
	qrtc_get = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_set, qrtc_get, "QRTC get/set value mismatch");
}

void test_lcz_qrtc_tm_set_get(void)
{
	/* LCZ QRTC Test 3:
	 *   Check set (via tm) and get epoch functions with various input values
	 */
	uint32_t qrtc_actual, qrtc_actual_set, qrtc_expected;
	int32_t qrtc_offset;
	struct tm time_field;

	/* Valid date with no offset */
	time_field.tm_sec = 13;
	time_field.tm_min = 26;
	time_field.tm_hour = 3;
	time_field.tm_mday = 5;
	time_field.tm_mon = LCZ_QRTC_MONTH_JANUARY;
	time_field.tm_year = 2001;
	qrtc_offset = 0;
	qrtc_expected = 978665173 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid date with negative offset */
	time_field.tm_sec = 13;
	time_field.tm_min = 26;
	time_field.tm_hour = 3;
	time_field.tm_mday = 5;
	time_field.tm_mon = LCZ_QRTC_MONTH_JANUARY;
	time_field.tm_year = 2001;
	qrtc_offset = -3600;
	qrtc_expected = 978665173 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid date with positive offset */
	time_field.tm_sec = 13;
	time_field.tm_min = 26;
	time_field.tm_hour = 3;
	time_field.tm_mday = 5;
	time_field.tm_mon = LCZ_QRTC_MONTH_JANUARY;
	time_field.tm_year = 2001;
	qrtc_offset = 30;
	qrtc_expected = 978665173 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid date with no offset */
	time_field.tm_sec = 22;
	time_field.tm_min = 22;
	time_field.tm_hour = 22;
	time_field.tm_mday = 5;
	time_field.tm_mon = LCZ_QRTC_MONTH_FEBRUARY;
	time_field.tm_year = 2008;
	qrtc_offset = 0;
	qrtc_expected = 1202250142 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid date with positive offset */
	time_field.tm_sec = 0;
	time_field.tm_min = 1;
	time_field.tm_hour = 2;
	time_field.tm_mday = 4;
	time_field.tm_mon = LCZ_QRTC_MONTH_MARCH;
	time_field.tm_year = 2031;
	qrtc_offset = 16;
	qrtc_expected = 1930356060 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid date with no offset */
	time_field.tm_sec = 8;
	time_field.tm_min = 4;
	time_field.tm_hour = 2;
	time_field.tm_mday = 1;
	time_field.tm_mon = LCZ_QRTC_MONTH_APRIL;
	time_field.tm_year = 1970;
	qrtc_offset = 0;
	qrtc_expected = 7783448 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid date with no offset */
	time_field.tm_sec = 44;
	time_field.tm_min = 11;
	time_field.tm_hour = 3;
	time_field.tm_mday = 11;
	time_field.tm_mon = LCZ_QRTC_MONTH_MAY;
	time_field.tm_year = 2000;
	qrtc_offset = 0;
	qrtc_expected = 958014704 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Invalid date (prior to epoch) with positive offset - expected to fail */
	time_field.tm_sec = 59;
	time_field.tm_min = 59;
	time_field.tm_hour = 23;
	time_field.tm_mday = 27;
	time_field.tm_mon = LCZ_QRTC_MONTH_JUNE;
	time_field.tm_year = 1950;
	qrtc_offset = 2;
	/* qrtc_expected not set as expected failure */
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid date with no offset */
	time_field.tm_sec = 0;
	time_field.tm_min = 1;
	time_field.tm_hour = 0;
	time_field.tm_mday = 1;
	time_field.tm_mon = LCZ_QRTC_MONTH_JULY;
	time_field.tm_year = 2020;
	qrtc_offset = 0;
	qrtc_expected = 1593561660 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Invalid date (prior to epoch) with no offset - expected to fail */
	time_field.tm_sec = 1;
	time_field.tm_min = 0;
	time_field.tm_hour = 0;
	time_field.tm_mday = 2;
	time_field.tm_mon = LCZ_QRTC_MONTH_AUGUST;
	time_field.tm_year = 1900;
	qrtc_offset = 0;
	/* qrtc_expected not set as expected failure */
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid date with no offset */
	time_field.tm_sec = 0;
	time_field.tm_min = 0;
	time_field.tm_hour = 1;
	time_field.tm_mday = 4;
	time_field.tm_mon = LCZ_QRTC_MONTH_SEPTEMBER;
	time_field.tm_year = 2000;
	qrtc_offset = 0;
	qrtc_expected = 968029200 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid date past 2038 (signed 32-bit epoch overflow) with positive
	 * offset
	 */
	time_field.tm_sec = 0;
	time_field.tm_min = 0;
	time_field.tm_hour = 0;
	time_field.tm_mday = 1;
	time_field.tm_mon = LCZ_QRTC_MONTH_OCTOBER;
	time_field.tm_year = 2050;
	qrtc_offset = 3;
	qrtc_expected = 2548195200 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid date past 2038 (signed 32-bit epoch overflow) with no offset */
	time_field.tm_sec = 4;
	time_field.tm_min = 3;
	time_field.tm_hour = 2;
	time_field.tm_mday = 1;
	time_field.tm_mon = LCZ_QRTC_MONTH_NOVEMBER;
	time_field.tm_year = 2100;
	qrtc_offset = 0;
	qrtc_expected = 4128717784 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid date with no offset */
	time_field.tm_sec = 8;
	time_field.tm_min = 4;
	time_field.tm_hour = 2;
	time_field.tm_mday = 1;
	time_field.tm_mon = LCZ_QRTC_MONTH_DECEMBER;
	time_field.tm_year = 2000;
	qrtc_offset = 0;
	qrtc_expected = 975636248 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid date with no offset */
	time_field.tm_sec = 1;
	time_field.tm_min = 51;
	time_field.tm_hour = 18;
	time_field.tm_mday = 16;
	time_field.tm_mon = LCZ_QRTC_MONTH_MARCH;
	time_field.tm_year = 1999;
	qrtc_offset = 0;
	qrtc_expected = 921610261 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");
}

void test_lcz_qrtc_tm_set_get_offset(void)
{
	/* LCZ QRTC Test 4:
	 *   Check set (via tm) with large offset values
	 */
	uint32_t qrtc_actual, qrtc_actual_set, qrtc_expected;
	int32_t qrtc_offset;
	struct tm time_field;

	/* Valid date with no offset */
	time_field.tm_sec = 10;
	time_field.tm_min = 22;
	time_field.tm_hour = 6;
	time_field.tm_mday = 2;
	time_field.tm_mon = LCZ_QRTC_MONTH_MARCH;
	time_field.tm_year = 1992;
	qrtc_offset = 0;
	qrtc_expected = 699517330 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid small date with small (larger than date) offset - expected fail */
	time_field.tm_sec = 10;
	time_field.tm_min = 22;
	time_field.tm_hour = 6;
	time_field.tm_mday = 2;
	time_field.tm_mon = LCZ_QRTC_MONTH_JANUARY;
	time_field.tm_year = 1970;
	qrtc_offset = -109390;
	/* qrtc_expected not set as expected failure */
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();

	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid small date with small (larger than date) offset */
	time_field.tm_sec = 2;
	time_field.tm_min = 2;
	time_field.tm_hour = 2;
	time_field.tm_mday = 1;
	time_field.tm_mon = LCZ_QRTC_MONTH_JANUARY;
	time_field.tm_year = 1970;
	qrtc_offset = -9000;
	qrtc_expected = 7322 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid small date with small (larger than date) offset - expected fail */
	time_field.tm_sec = 2;
	time_field.tm_min = 2;
	time_field.tm_hour = 2;
	time_field.tm_mday = 1;
	time_field.tm_mon = LCZ_QRTC_MONTH_JANUARY;
	time_field.tm_year = 1970;
	qrtc_offset = 9000;
	qrtc_expected = 7322 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_not_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid date with large offset (less than epoch) - expected fail */
	time_field.tm_sec = 10;
	time_field.tm_min = 22;
	time_field.tm_hour = 6;
	time_field.tm_mday = 2;
	time_field.tm_mon = LCZ_QRTC_MONTH_MARCH;
	time_field.tm_year = 1995;
	qrtc_offset = 794125300;
	qrtc_expected = 794125330 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_not_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid date with no offset */
	time_field.tm_sec = 10;
	time_field.tm_min = 22;
	time_field.tm_hour = 6;
	time_field.tm_mday = 2;
	time_field.tm_mon = LCZ_QRTC_MONTH_MARCH;
	time_field.tm_year = 1992;
	qrtc_offset = 0;
	qrtc_expected = 699517330 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid date with large offset (more than epoch) - expected fail */
	time_field.tm_sec = 10;
	time_field.tm_min = 22;
	time_field.tm_hour = 6;
	time_field.tm_mday = 2;
	time_field.tm_mon = LCZ_QRTC_MONTH_MARCH;
	time_field.tm_year = 1995;
	qrtc_offset = 794125390;
	/* qrtc_expected not set as expected failure */
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid date with no offset */
	time_field.tm_sec = 10;
	time_field.tm_min = 22;
	time_field.tm_hour = 6;
	time_field.tm_mday = 2;
	time_field.tm_mon = LCZ_QRTC_MONTH_MARCH;
	time_field.tm_year = 1992;
	qrtc_offset = 0;
	qrtc_expected = 699517330 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid date with large negative offset (less than epoch) - expected fail */
	time_field.tm_sec = 10;
	time_field.tm_min = 22;
	time_field.tm_hour = 6;
	time_field.tm_mday = 2;
	time_field.tm_mon = LCZ_QRTC_MONTH_MARCH;
	time_field.tm_year = 1995;
	qrtc_offset = -794125300;
	/* qrtc_expected not set as expected failure */
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid date with no offset */
	time_field.tm_sec = 10;
	time_field.tm_min = 22;
	time_field.tm_hour = 6;
	time_field.tm_mday = 2;
	time_field.tm_mon = LCZ_QRTC_MONTH_MARCH;
	time_field.tm_year = 1992;
	qrtc_offset = 0;
	qrtc_expected = 699517330 - qrtc_offset;
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	/* Valid date with large negative offset (more than epoch) - expected fail */
	time_field.tm_sec = 10;
	time_field.tm_min = 22;
	time_field.tm_hour = 6;
	time_field.tm_mday = 2;
	time_field.tm_mon = LCZ_QRTC_MONTH_MARCH;
	time_field.tm_year = 1995;
	qrtc_offset = -794125390;
	/* qrtc_expected not set as expected failure */
	update_tm_year(&time_field);
	qrtc_actual_set = lcz_qrtc_set_epoch_from_tm(&time_field, qrtc_offset);
	qrtc_actual = lcz_qrtc_get_epoch();
	zassert_equal(qrtc_actual_set, qrtc_actual,
		      "QRTC tm get/set value mismatch");
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");
}

void test_lcz_qrtc_increment(void)
{
	/* LCZ QRTC Test 5:
	 *   Check epoch value increments
	 */
	uint32_t qrtc_actual, qrtc_expected;
	uint8_t i;

	qrtc_actual = 1234;
	qrtc_actual = lcz_qrtc_set_epoch(qrtc_actual);
	qrtc_expected = qrtc_actual + LCZ_QRTC_SLEEP_TIME_1S;

	i = 0;
	while (i < 11) {
		k_sleep(K_SECONDS(LCZ_QRTC_SLEEP_TIME_1S));
		qrtc_actual = lcz_qrtc_get_epoch();
		zassert_equal(qrtc_actual, qrtc_expected,
			      "QRTC increment value mismatch");
		++i;
		if (i < 11) {
			qrtc_expected += LCZ_QRTC_SLEEP_TIME_1S;
		}
	}

	k_sleep(K_SECONDS(LCZ_QRTC_SLEEP_TIME_3S));
	qrtc_actual = lcz_qrtc_get_epoch();
	qrtc_expected += LCZ_QRTC_SLEEP_TIME_3S;
	zassert_equal(qrtc_actual, qrtc_expected,
		      "QRTC increment value mismatch");

	k_sleep(K_SECONDS(LCZ_QRTC_SLEEP_TIME_7S));
	qrtc_actual = lcz_qrtc_get_epoch();
	qrtc_expected += LCZ_QRTC_SLEEP_TIME_7S;
	zassert_equal(qrtc_actual, qrtc_expected,
		      "QRTC increment value mismatch");
}

void test_lcz_qrtc_tm_increment(void)
{
	/* LCZ QRTC Test 6:
	 *   Check epoch (set with tm) value increments
	 */
	time_t time_buf;
	struct tm time_field, check_time_field;
	uint32_t qrtc_actual, qrtc_expected;
	uint8_t i;

	time_field.tm_sec = 56;
	time_field.tm_min = 59;
	time_field.tm_hour = 23;
	time_field.tm_mday = 1;
	time_field.tm_mon = LCZ_QRTC_MONTH_MARCH;
	time_field.tm_year = 1999;
	update_tm_year(&time_field);
	qrtc_expected = 920332796;
	qrtc_actual = lcz_qrtc_set_epoch_from_tm(&time_field, 0);
	zassert_equal(qrtc_expected, qrtc_actual, "QRTC tm value not valid");

	qrtc_expected = qrtc_expected + LCZ_QRTC_SLEEP_TIME_1S;

	i = 0;
	while (i < 5) {
		k_sleep(K_SECONDS(LCZ_QRTC_SLEEP_TIME_1S));
		qrtc_actual = lcz_qrtc_get_epoch();
		zassert_equal(qrtc_actual, qrtc_expected,
			      "QRTC tm increment value mismatch");
		++i;
		if (i < 5) {
			qrtc_expected += LCZ_QRTC_SLEEP_TIME_1S;
		}
	}

	k_sleep(K_SECONDS(LCZ_QRTC_SLEEP_TIME_3S));
	qrtc_actual = lcz_qrtc_get_epoch();
	qrtc_expected += LCZ_QRTC_SLEEP_TIME_3S;
	zassert_equal(qrtc_actual, qrtc_expected,
		      "QRTC tm increment value mismatch");

	time_buf = qrtc_actual;
	gmtime_r(&time_buf, &check_time_field);

	time_field.tm_sec = 4;
	time_field.tm_min = 0;
	time_field.tm_hour = 0;
	time_field.tm_mday = 2;
	time_field.tm_mon = LCZ_QRTC_MONTH_MARCH;
	time_field.tm_year = 1999;
	update_tm_year(&time_field);

	/* Clear fields that are not used */
	check_time_field.tm_wday = 0;
	check_time_field.tm_yday = 0;
	check_time_field.tm_isdst = 0;
	time_field.tm_wday = 0;
	time_field.tm_yday = 0;
	time_field.tm_isdst = 0;

	zassert_mem_equal(&check_time_field, &time_field, sizeof(struct tm),
			  "QRTC tm time struct value mismatch");

	qrtc_actual = lcz_qrtc_get_epoch();
	qrtc_expected = 920332796 + 3 + 5;
	zassert_equal(qrtc_actual, qrtc_expected,
		      "QRTC tm increment value mismatch");
}

void test_lcz_qrtc_is_set(void)
{
	/* LCZ QRTC Test 7:
	 *   Check QRTC has been set
	 */
	lcz_qrtc_set_epoch(300);
	zassert_true(lcz_qrtc_epoch_was_set(),
		     "QRTC is not set when it should be");
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static void update_tm_year(struct tm *time_field)
{
	time_field->tm_year -= 1900;
}
