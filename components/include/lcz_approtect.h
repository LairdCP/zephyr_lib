/**
 * @file lcz_approtect.h
 * @brief APPROTECT enablement for nRF52 silicon
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __APPROTECT_H__
#define __APPROTECT_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Enables readback protection, will reboot the module if it is not
 * currently enabled (and requested) otherwise will return execution to the
 * calling function
 *
 * @param true to restart module if required, false otherwise
 *
 * @retval true if change was applied, false is no update was required
 */
bool lcz_enable_readback_protection(bool restart);

/**
 * @brief Enables CPU debug protection, will reboot the module if it is not
 * currently enabled (and requested) otherwise will return execution to the
 * calling function
 *
 * @param true to restart module if required, false otherwise
 *
 * @retval true if change was applied, false is no update was required
 */
bool lcz_enable_cpu_debug_readback_protection(bool restart);

/**
 * @brief Enables CPU debug and readback protection, will reboot the module if
 * one is not currently enabled (and requested) otherwise will return execution
 * to the calling function
 *
 * @param true to restart module if required, false otherwise
 *
 * @retval true if change was applied, false is no update was required
 */
bool lcz_enable_cpu_debug_protection(bool restart);

#ifdef __cplusplus
}
#endif

#endif /* __APPROTECT_H__ */
