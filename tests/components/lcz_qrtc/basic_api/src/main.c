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
	ztest_test_suite(lcz_qrtc_basic_api_test,
			 ztest_unit_test(test_lcz_qrtc_is_not_set),
			 ztest_unit_test(test_lcz_qrtc_set_get),
			 ztest_unit_test(test_lcz_qrtc_tm_set_get),
			 ztest_unit_test(test_lcz_qrtc_tm_set_get_offset),
			 ztest_unit_test(test_lcz_qrtc_increment),
			 ztest_unit_test(test_lcz_qrtc_tm_increment),
			 ztest_unit_test(test_lcz_qrtc_is_set));
	ztest_run_test_suite(lcz_qrtc_basic_api_test);
}
