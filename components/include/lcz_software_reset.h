/**
 * @file lcz_software_reset.h
 * @brief Wraps sys reboot and memfault assert.
 *
 * Copyright (c) 2021-2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_SOFTWARE_RESET_H__
#define __LCZ_SOFTWARE_RESET_H__

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
 * @brief Call log panic. Reset after delay unless called from interrupt
 * context.
 */
void lcz_software_reset(uint32_t delay_ms);

/**
 * @brief Call log panic. If memfault is enabled, then use
 * memfault assert to generate core dump and reset.
 *
 * @param delay_ms is not used when memfault is enabled or when called
 * from interrupt context.
 */
void lcz_software_reset_after_assert(uint32_t delay_ms);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_SOFTWARE_RESET_H__ */
