/**
 * @file string_util.h
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __STRING_UTIL_H__
#define __STRING_UTIL_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
void strncpy_replace_underscore_with_space(char *restrict s1,
					   const char *restrict s2,
					   size_t size);

char *replace_word(const char *s, const char *oldW, const char *newW,
		   char *dest, int destSize);

char *strncat_max(char *d1, const char *s1, size_t max_str_len);

#ifdef __cplusplus
}
#endif

#endif /* __STRING_UTILITES_H__ */
