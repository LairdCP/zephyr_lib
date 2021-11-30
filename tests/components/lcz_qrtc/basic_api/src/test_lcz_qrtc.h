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
void test_lcz_qrtc_is_not_set(void);
void test_lcz_qrtc_set_get(void);
void test_lcz_qrtc_tm_set_get(void);
void test_lcz_qrtc_tm_set_get_offset(void);
void test_lcz_qrtc_increment(void);
void test_lcz_qrtc_tm_increment(void);
void test_lcz_qrtc_is_set(void);

#endif /* __TEST_QRTC_H__ */
