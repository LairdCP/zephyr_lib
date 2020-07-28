/**
 * @file laird_connectivity_hwrev.h
 * @brief Hardware revision identification.
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __HWREV_H__
#define __HWREV_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Board definitions                                                          */
/******************************************************************************/

#define HWREV_ID1_BIT_VALUE      1
#define HWREV_ID2_BIT_VALUE      2
#define HWREV_ID3_BIT_VALUE      4
#define HWREV_VERSION_UNKNOWN    -1
#define HWREV_VERSION_VALID_MIN  0
#define HWREV_VERSION_VALID_MAX  7

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/

/**
 * @brief returns the hardware revision
 *
 * @return HWREV_VERSION_UNKNOWN if not known, otherwise the version
 */

int8_t hwrev_get_version();

/**
 * @brief initialise and get the hardware revision
 *
 * @return 0 for success
 */

int hwrev_init();

#ifdef __cplusplus
}
#endif

#endif /* __HWREV_H__ */
