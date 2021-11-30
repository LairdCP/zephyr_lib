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
#include "test_lcz_qrtc.h"

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void test_main(void)
{
	lcz_qrtc_test_setup();

	ztest_test_suite(lcz_qrtc_sync_test,
			 ztest_unit_test(test_lcz_qrtc_sync));
	ztest_run_test_suite(lcz_qrtc_sync_test);
}
