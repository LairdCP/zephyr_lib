/**
 * @file print_thread.h
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __PRINT_THREAD_H__
#define __PRINT_THREAD_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/

/**
 * @brief Prints list of threads using k_thread_foreach.
 */
void print_thread_list(void);

#ifdef __cplusplus
}
#endif

#endif /* __PRINT_THREAD_H__ */
