/**
 * @file lcz_software_reset.h
 * @brief Wraps sys reboot.
 *
 * Copyright (c) 2021 Laird Connectivity
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
 * @brief Call log panic. Reset after delay.
 */
void lcz_software_reset(uint32_t delay_ms);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_SOFTWARE_RESET_H__ */
