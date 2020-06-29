/**
 * @file hexcode.h
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __HEXCODE_H__
#define __HEXCODE_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/

/* clang-format off */
#define HEX_DECODE_LOWER_CASE_ALPHA_SUBTRACT 0x57
#define HEX_DECODE_UPPER_CASE_ALPHA_SUBTRACT 0x37
#define HEX_DECODE_NUMERIC_SUBTRACT          0x30
#define HEX_ENCODE_LOWER_CASE_ALPHA_ADDITION 0x27
#define HEX_ENCODE_UPPER_CASE_ALPHA_ADDITION 0x7
#define HEX_ENCODE_NUMERIC_ADDITION          0x30
/* clang-format on */

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void HexEncode(uint8_t *pInput, uint32_t nLength, uint8_t *pOutput,
	       bool bUpperCase, bool bWithNullTermination);

#ifdef __cplusplus
}
#endif

#endif /* __HEXCODE_H__ */
