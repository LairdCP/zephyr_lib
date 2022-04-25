/**
 * @file errno_str.h
 * @brief
 *
 * Copyright (c) 2021-2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ERRNO_STR_H__
#define __ERRNO_STR_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Constants, Macros and type Definitions                              */
/******************************************************************************/
#define ERRNO_STR(_e) (((_e) == 0) ? "OK" : errno_str_get(_e))

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
