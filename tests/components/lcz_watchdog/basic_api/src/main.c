/**
 * @file main.c
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include "test_lcz_wdt.h"

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void test_main(void)
{
	ztest_test_suite(lcz_wdt_basic_api_test,
			 ztest_unit_test(test_lcz_wdt_basic_api));
	ztest_run_test_suite(lcz_wdt_basic_api_test);
}
