/**
 * @file errno_str.h
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ERRNO_STR_H__
#define __ERRNO_STR_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Get string representation of errno
 *
 * @param err negative error code
 * @return const char*
 */
const char *errno_str_get(int err);

#ifdef __cplusplus
}
#endif

#endif /* __ERRNO_STR_H__ */
