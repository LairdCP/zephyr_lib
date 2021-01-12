/**
 * @file lcz_memfault.h
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_MEMFAULT_H__
#define __LCZ_MEMFAULT_H__

/* (Remove Empty Sections) */
/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include "memfault/core/build_info.h"
#include "memfault/core/platform/device_info.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/

/**
 * @brief Initialize memfault SDK
 * 
 * @param api_key API key to communicate to Memfault
 * @return 0 on success 
 */
int lcz_memfault_init(char *api_key);

/**
 * @brief Post any available data to memfault cloud via HTTPS
 * 
 * @return 0 on success 
 */
int lcz_memfault_post_data();

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_MEMFAULT_H__ */
