/**
 * @file lcz_nrf_reset_reason.h
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_NRF_RESET_REASON__
#define __LCZ_NRF_RESET_REASON__

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
 * @retval Reset reason code
 */
uint32_t lcz_nrf_reset_reason_get_and_clear_register(void);

/**
 * @brief The reset reason register can have multiple bits set.
 * The reset reason should be cleared after it is used to prevent this.
 * This function returns the code for the first bit that is set in its
 * prioritized list.
 *
 * @retval Reset reason code as a string (from reset reason register).
 */
const char *lcz_nrf_reset_reason_get_string(uint32_t reg);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_NRF_RESET_REASON__ */
