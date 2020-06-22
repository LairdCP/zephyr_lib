/**
 * @file print_json.c
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <sys/printk.h>
#include "print_json.h"

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
/* Log system cannot be used to print JSON buffers because they are too large.
 * String could be broken into LOG_STRDUP_MAX_STRING chunks.
 */
void print_json(const char *prefix, size_t size, const char *buffer)
{
#if CONFIG_JSON_LOG_ENABLED
	printk("%s size: %u data: %s\r\n", prefix, size, buffer);
#endif
}