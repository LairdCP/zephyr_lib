/**
 * @file test_lcz_wdt_simple_api.c
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
#include "test_lcz_wdt.h"
#include "lcz_watchdog.h"
#include "lcz_nrf_reset_reason.h"
#include "helpers/nrfx_reset_reason.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay)
#define NOINIT_SECTION ".dtcm_noinit.test_lcz_wdt"
#else
#define NOINIT_SECTION ".noinit.test_lcz_wdt"
#endif

#define NON_INIT_MAGIC_VALUE 0x3194

/**
 * @note Items in non-initialized RAM need to survive a reset.
 */
typedef struct no_init_ram {
        uint16_t header;
        uint16_t test_number;
        uint16_t passes;
} no_init_ram_t;

typedef enum {
	LCZ_WDT_TEST_NUMBER_1 = 0,
	LCZ_WDT_TEST_NUMBER_2,
	LCZ_WDT_TEST_NUMBER_3,
	LCZ_WDT_TEST_NUMBER_4,
	LCZ_WDT_TEST_NUMBER_5,
	LCZ_WDT_TEST_NUMBER_6,
	LCZ_WDT_TEST_NUMBER_7,
	LCZ_WDT_TEST_NUMBER_8,
	LCZ_WDT_TEST_NUMBER_9
} lcz_wdt_test_number_t;

typedef enum {
	LCZ_WDT_PASS_TEST_A = 0x1,
	LCZ_WDT_PASS_TEST_B = 0x2,
	LCZ_WDT_PASS_TEST_C = 0x4,
	LCZ_WDT_PASS_TEST_D = 0x8,
	LCZ_WDT_PASS_TEST_E = 0x10,
	LCZ_WDT_PASS_TEST_F = 0x20,
	LCZ_WDT_PASS_TEST_G = 0x40
} lcz_wdt_pass_test_t;

typedef enum {
	LCZ_WDT_SLEEP_TIME_1S = 1,
	LCZ_WDT_SLEEP_TIME_2S = 2,
	LCZ_WDT_SLEEP_TIME_6S = 6,
	LCZ_WDT_SLEEP_TIME_10S = 10
} lcz_wdt_sleep_time_t;

#define SLEEP_TIME_10S_MS 10000

#define LCZ_WDT_PASS_SUCCESS_BITMASK LCZ_WDT_PASS_TEST_A | \
				     LCZ_WDT_PASS_TEST_B | \
				     LCZ_WDT_PASS_TEST_C | \
				     LCZ_WDT_PASS_TEST_D | \
				     LCZ_WDT_PASS_TEST_E | \
				     LCZ_WDT_PASS_TEST_F | \
				     LCZ_WDT_PASS_TEST_G

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
volatile no_init_ram_t pnird __attribute__((section(NOINIT_SECTION)));

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static bool reset_by_wdog(uint32_t reset_reason);
static void set_non_init_header(void);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void test_lcz_wdt_basic_api(void)
{
	int rc;
	int8_t prev_test = -1;
	int user_id_a = -1;
	int user_id_b = -1;
	uint32_t reset_reason = lcz_nrf_reset_reason_get_and_clear_register();
	int64_t uptime;

	/* Read in test details from non-initialised RAM */
	if (pnird.header == NON_INIT_MAGIC_VALUE) {
		pnird.header = 0;
	} else {
		pnird.header = 0;
		pnird.test_number = LCZ_WDT_TEST_NUMBER_1;
		pnird.passes = 0;
	}

	/* LCZ_Watchdog Test 1:
	 *   Check watchdog does not reset module if no users are configured
	 */
	if (pnird.test_number == LCZ_WDT_TEST_NUMBER_1) {
		/* Check reset reason is not due to the watchdog */
		zassert_equal(prev_test, -1, "No previous test should have ran");
		zassert_equal(reset_by_wdog(reset_reason), false,
			      "Reset reason should not be due to watchdog");

		set_non_init_header();

		k_sleep(K_SECONDS(LCZ_WDT_SLEEP_TIME_6S));

		pnird.passes |= LCZ_WDT_PASS_TEST_A;
		pnird.test_number = LCZ_WDT_TEST_NUMBER_2;
		prev_test = LCZ_WDT_TEST_NUMBER_1;
		set_non_init_header();
	}

	/* LCZ_Watchdog Test 2:
	 *   Check watchdog resets module if forced to time out
	 */
	if (pnird.test_number == LCZ_WDT_TEST_NUMBER_2) {
		/* Check reset reason is not due to the watchdog */
		zassert_equal(prev_test, LCZ_WDT_TEST_NUMBER_1,
			      "Previous test value is invalid");
		zassert_equal(reset_by_wdog(reset_reason), false,
			      "Reset reason should not be due to watchdog");

		pnird.test_number = LCZ_WDT_TEST_NUMBER_3;
		set_non_init_header();

		rc = lcz_wdt_force();
		zassert_equal(rc, 0, "Watchdog force timeout failed");

		k_sleep(K_SECONDS(LCZ_WDT_SLEEP_TIME_6S));

		/* This should not execute as the watchdog should reset before
		 * this point
		 */
		zassert_unreachable("Watchdog should have rebooted module");

		prev_test = LCZ_WDT_TEST_NUMBER_2;
		pnird.passes = 0;
		set_non_init_header();
	}

	/* LCZ_Watchdog Test 3:
	 *   Check watchdog does not reset module if a user that checks in is
	 *   registered
	 */
	if (pnird.test_number == LCZ_WDT_TEST_NUMBER_3) {
		/* Check reset reason is due to the watchdog */
		zassert_equal(prev_test, -1,
			      "Previous test value should not be set");
		zassert_equal(reset_by_wdog(reset_reason), true,
			      "Reset reason should be due to watchdog");
		user_id_a = lcz_wdt_get_user_id();
		zassert_equal(user_id_a, 1, "Watchdog user ID should be 1");

		pnird.test_number = LCZ_WDT_TEST_NUMBER_4;
		set_non_init_header();

		rc = lcz_wdt_check_in(user_id_a);
		zassert_ok(rc, "WDT check in was not successful");
		k_sleep(K_SECONDS(LCZ_WDT_SLEEP_TIME_2S));
		rc = lcz_wdt_check_in(user_id_a);
		zassert_ok(rc, "WDT check in was not successful");
		k_sleep(K_SECONDS(LCZ_WDT_SLEEP_TIME_1S));
		rc = lcz_wdt_pause(user_id_a);
		zassert_ok(rc, "WDT pause was not successful");
		k_sleep(K_SECONDS(LCZ_WDT_SLEEP_TIME_2S));
		rc = lcz_wdt_check_in(user_id_a);
		zassert_ok(rc, "WDT check in was not successful");
		k_sleep(K_SECONDS(LCZ_WDT_SLEEP_TIME_1S));
		rc = lcz_wdt_check_in(user_id_a);
		zassert_ok(rc, "WDT check in was not successful");
		k_sleep(K_SECONDS(LCZ_WDT_SLEEP_TIME_2S));
		prev_test = LCZ_WDT_TEST_NUMBER_3;

		pnird.passes |= LCZ_WDT_PASS_TEST_B;
		set_non_init_header();
	}

	/* LCZ_Watchdog Test 4:
	 *   Check watchdog resets module if a user that is registered stops
	 *   checking in
	 */
	if (pnird.test_number == LCZ_WDT_TEST_NUMBER_4) {
		if (prev_test != LCZ_WDT_TEST_NUMBER_3) {
			/* Check reset reason is not due to the watchdog */
			pnird.passes = 0;
			set_non_init_header();
			zassert_equal(reset_by_wdog(reset_reason), false,
				      "Reset reason should not be due to watchdog");
		}

		zassert_equal(prev_test, LCZ_WDT_TEST_NUMBER_3,
			      "Previous test value is invalid");

		pnird.test_number = LCZ_WDT_TEST_NUMBER_5;
		set_non_init_header();

		rc = lcz_wdt_check_in(user_id_a);
		zassert_ok(rc, "WDT check in was not successful");
		k_sleep(K_SECONDS(LCZ_WDT_SLEEP_TIME_6S));

		zassert_unreachable("Watchdog should have rebooted module");
		prev_test = LCZ_WDT_TEST_NUMBER_4;
		pnird.passes = 0;
		set_non_init_header();
	}

	/* LCZ_Watchdog Test 5:
	 *   Check watchdog with 2 registered users that check in does not
	 *   reset
	 */
	if (pnird.test_number == LCZ_WDT_TEST_NUMBER_5) {
		/* Check reset reason is due to the watchdog */
		if (reset_by_wdog(reset_reason) != true) {
			pnird.passes = 0;
			set_non_init_header();
		}

		zassert_equal(reset_by_wdog(reset_reason), true,
			      "Reset reason should be due to watchdog");

		pnird.test_number = LCZ_WDT_TEST_NUMBER_6;
		set_non_init_header();

		user_id_a = lcz_wdt_get_user_id();
		zassert_equal(user_id_a, 1, "Watchdog user ID A should be 1");
		user_id_b = lcz_wdt_get_user_id();
		zassert_equal(user_id_b, 2, "Watchdog user ID B should be 2");

		rc = lcz_wdt_check_in(user_id_a);
		zassert_ok(rc, "WDT check in A was not successful");
		rc = lcz_wdt_check_in(user_id_b);
		zassert_ok(rc, "WDT check in B was not successful");
		k_sleep(K_SECONDS(LCZ_WDT_SLEEP_TIME_2S));
		rc = lcz_wdt_check_in(user_id_a);
		zassert_ok(rc, "WDT check in A was not successful");
		rc = lcz_wdt_check_in(user_id_b);
		zassert_ok(rc, "WDT check in B was not successful");
		k_sleep(K_SECONDS(LCZ_WDT_SLEEP_TIME_1S));
		rc = lcz_wdt_pause(user_id_a);
		zassert_ok(rc, "WDT pause A was not successful");
		rc = lcz_wdt_check_in(user_id_b);
		zassert_ok(rc, "WDT check in B was not successful");
		k_sleep(K_SECONDS(LCZ_WDT_SLEEP_TIME_2S));
		rc = lcz_wdt_check_in(user_id_a);
		zassert_ok(rc, "WDT check in A was not successful");
		rc = lcz_wdt_check_in(user_id_b);
		zassert_ok(rc, "WDT check in B was not successful");
		k_sleep(K_SECONDS(LCZ_WDT_SLEEP_TIME_1S));
		rc = lcz_wdt_check_in(user_id_a);
		zassert_ok(rc, "WDT check in A was not successful");
		rc = lcz_wdt_pause(user_id_b);
		zassert_ok(rc, "WDT pause B was not successful");
		k_sleep(K_SECONDS(LCZ_WDT_SLEEP_TIME_2S));
		rc = lcz_wdt_check_in(user_id_a);
		zassert_ok(rc, "WDT check in A was not successful");
		rc = lcz_wdt_check_in(user_id_b);
		zassert_ok(rc, "WDT check in B was not successful");
		k_sleep(K_SECONDS(LCZ_WDT_SLEEP_TIME_1S));

		prev_test = LCZ_WDT_TEST_NUMBER_5;
		pnird.passes |= LCZ_WDT_PASS_TEST_C;
		set_non_init_header();
	}

	/* LCZ_Watchdog Test 6:
	 *   Check watch with 2 registered users that do not check in resets
	 *   module
	 */
	if (pnird.test_number == LCZ_WDT_TEST_NUMBER_6) {
		if (prev_test != LCZ_WDT_TEST_NUMBER_5) {
			pnird.passes = 0;
			set_non_init_header();

			/* Check reset reason is not due to the watchdog */
			zassert_equal(reset_by_wdog(reset_reason), false,
				      "Reset reason should not be due to watchdog");
		}

		zassert_equal(prev_test, LCZ_WDT_TEST_NUMBER_5,
			      "Previous test number should be 4");

		pnird.test_number = LCZ_WDT_TEST_NUMBER_7;
		set_non_init_header();

		k_sleep(K_SECONDS(LCZ_WDT_SLEEP_TIME_6S));

		prev_test = LCZ_WDT_TEST_NUMBER_6;
		pnird.passes = 0;
		set_non_init_header();
	}

	/* LCZ_Watchdog Test 7:
	 *   Check invalid user IDs with pause/check in functions
	 */
	if (pnird.test_number == LCZ_WDT_TEST_NUMBER_7) {
		/* Check reset reason is due to the watchdog */
		if (reset_by_wdog(reset_reason) != true) {
			pnird.passes = 0;
			set_non_init_header();
		} else {
			pnird.passes |= LCZ_WDT_PASS_TEST_D;
		}

		zassert_equal(prev_test, -1,
			      "Previous test number should be unset");
		zassert_equal(reset_by_wdog(reset_reason), true,
			      "Reset reason should be due to watchdog");

		user_id_a = 33;
		rc = lcz_wdt_check_in(user_id_a);
		zassert_not_equal(rc, 0,
				  "WDT check in with invalid ID did not return an error");
		rc = lcz_wdt_pause(user_id_b);
		zassert_not_equal(rc, 0,
				  "WDT pause with invalid ID did not return an error");

		user_id_a = 65;
		rc = lcz_wdt_check_in(user_id_a);
		zassert_not_equal(rc, 0,
				  "WDT check in with invalid ID did not return an error");
		rc = lcz_wdt_pause(user_id_b);
		zassert_not_equal(rc, 0,
				  "WDT pause with invalid ID did not return an error");

		user_id_a = 127;
		rc = lcz_wdt_check_in(user_id_a);
		zassert_not_equal(rc, 0,
				  "WDT check in with invalid ID did not return an error");
		rc = lcz_wdt_pause(user_id_b);
		zassert_not_equal(rc, 0,
				  "WDT pause with invalid ID did not return an error");

		user_id_a = 222;
		rc = lcz_wdt_check_in(user_id_a);
		zassert_not_equal(rc, 0,
				  "WDT check in with invalid ID did not return an error");
		rc = lcz_wdt_pause(user_id_b);
		zassert_not_equal(rc, 0,
				  "WDT pause with invalid ID did not return an error");

		prev_test = LCZ_WDT_TEST_NUMBER_7;
		pnird.passes |= LCZ_WDT_PASS_TEST_E;
		pnird.test_number = LCZ_WDT_TEST_NUMBER_8;
		set_non_init_header();
	}

	/* LCZ_Watchdog Test 8:
	 *   Check that module reboots during a busy-wait timeout
	 */
	if (pnird.test_number == LCZ_WDT_TEST_NUMBER_8) {
		if (prev_test == LCZ_WDT_TEST_NUMBER_7) {
			pnird.passes |= LCZ_WDT_PASS_TEST_F;
		} else {
			pnird.passes = 0;
		}

		set_non_init_header();

		zassert_equal(prev_test, LCZ_WDT_TEST_NUMBER_7,
			      "Previous test number should be set");

		pnird.test_number = LCZ_WDT_TEST_NUMBER_9;
		set_non_init_header();

		/* Wait for 10 seconds by which point the module should have
		 * been rebooted by the watchdog
		 */
		uptime = k_uptime_get();
		while (k_uptime_delta(&uptime) < SLEEP_TIME_10S_MS);

		/* This should not execute as the watchdog should reset before
		 * this point
		 */
		zassert_unreachable("Watchdog should have rebooted module");

		prev_test = LCZ_WDT_TEST_NUMBER_8;
		pnird.passes = 0;
		set_non_init_header();
	}

	if (pnird.test_number == LCZ_WDT_TEST_NUMBER_9) {
		/* Check reset reason is due to the watchdog */
		if (reset_by_wdog(reset_reason) != true) {
			pnird.passes = 0;
			set_non_init_header();
		} else {
			pnird.passes |= LCZ_WDT_PASS_TEST_G;
		}

		zassert_equal(prev_test, -1,
			      "Previous test number should not be set");
		zassert_equal(reset_by_wdog(reset_reason), true,
			      "Reset reason should be due to watchdog");
	}

	zassert_equal(pnird.passes, LCZ_WDT_PASS_SUCCESS_BITMASK,
		      "Test pass list does not match expected value");
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static bool reset_by_wdog(uint32_t reset_reason)
{
	if ((reset_reason & (NRFX_RESET_REASON_DOG_MASK)) != 0) {
		/* Device was reset via watchdog */
		return true;
	}

	return false;
}

static void set_non_init_header(void)
{
	pnird.header = NON_INIT_MAGIC_VALUE;
}
