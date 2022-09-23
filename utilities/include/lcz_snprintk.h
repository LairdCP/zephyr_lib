/**
 * @file lcz_snprintk.h
 * @brief Wrap snprintk with log messages. Print error if message size is too small.
 * If heap is enabled, allow version that uses malloc.
 *
 * Copyright (c) 2022 Laird Connectivity LLC
 *
 * SPDX-License-Identifier: LicenseRef-LairdConnectivity-Clause
 */

#ifndef __LCZ_SNPRINTK_H__
#define __LCZ_SNPRINTK_H__

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <zephyr.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************/
/* Global Constants, Macros and Type Definitions                                                  */
/**************************************************************************************************/
#define LCZ_SNPRINTK(buf, format, ...) lcz_snprintk(buf, sizeof(buf), format, __VA_ARGS__)

/**************************************************************************************************/
/* Global Function Prototypes                                                                     */
/**************************************************************************************************/
/**
 * @brief Format a message
 *
 * @param msg buffer
 * @param size of buffer
 * @param fmt string
 * @param ... variable list
 * @return int negative error code, actual length on success
 */
int lcz_snprintk(char *msg, size_t size, const char *fmt, ...);

/**
 * @brief Allocate and format a message
 *
 * @param msg pointer to buffer pointer
 * @param length of buffer if not NULL, negative or desired length on error
 * @param fmt string
 * @param ... variable list
 * @return int negative error code, actual length on success
 */
int lcz_snprintk_malloc(char **msg, int *length, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_SNPRINTK_H__ */
