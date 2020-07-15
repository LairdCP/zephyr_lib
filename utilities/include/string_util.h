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
/**
 * @brief Helper function
 */
void strncpy_replace_underscore_with_space(char *restrict s1,
					   const char *restrict s2,
					   size_t size);

/**
 * @brief Replaces word in a string
 */
char *replace_word(const char *s, const char *oldW, const char *newW,
		   char *dest, int dest_size);

/**
 * @brief Concatenates until string is max_str_len characters long.
 */
char *strncat_max(char *d1, const char *s1, size_t max_str_len);

/**
 * @brief Copy up to dest_size characters from source.  If null character
 * is found before dest_size characters are read, then fill with pad char.
 */
void left_justify(char *dest, const char *source, size_t dest_size, char pad);

#ifdef __cplusplus
}
#endif

#endif /* __STRING_UTILITES_H__ */
