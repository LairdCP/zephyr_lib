/**
 * @file test_lcz_qrtc.h
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __TEST_QRTC_H__
#define __TEST_QRTC_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr.h>
#include <sys/util.h>
#include <ztest.h>

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
void lcz_qrtc_test_setup(void);
void test_lcz_qrtc_sync(void);

#endif /* __TEST_QRTC_H__ */
