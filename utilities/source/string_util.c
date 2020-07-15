/**
 * @file string_util.c
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <string.h>
#include <stdbool.h>

#include "string_util.h"

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void strncpy_replace_underscore_with_space(char *restrict s1,
					   const char *restrict s2, size_t size)
{
	memset(s1, 0, size);
	size_t i;
	for (i = 0; i < size - 1; i++) {
		if (s2[i] == '\0') {
			break;
		} else if (s2[i] == '_') {
			s1[i] = ' ';
		} else {
			s1[i] = s2[i];
		}
	}
}

char *replace_word(const char *s, const char *oldW, const char *newW,
		   char *dest, int destSize)
{
	int i, cnt = 0;
	int newWlen = strlen(newW);
	int oldWlen = strlen(oldW);

	/* Counting the number of times old word
	*  occur in the string
	*/
	for (i = 0; s[i] != '\0'; i++) {
		if (strstr(&s[i], oldW) == &s[i]) {
			cnt++;

			/* Jumping to index after the old word. */
			i += oldWlen - 1;
		}
	}

	/* Make sure new string isn't too big */
	if ((i + cnt * (newWlen - oldWlen) + 1) > destSize) {
		return NULL;
	}

	i = 0;
	while (*s) {
		/* compare the substring with the result */
		if (strstr(s, oldW) == s) {
			strcpy(&dest[i], newW);
			i += newWlen;
			s += oldWlen;
		} else {
			dest[i++] = *s++;
		}
	}

	dest[i] = '\0';
	return dest;
}

/* max_str_len is the maximum length of the resulting string. */
char *strncat_max(char *d1, const char *s1, size_t max_str_len)
{
	size_t len = strlen(d1);
	size_t n = (len < max_str_len) ? (max_str_len - len) : 0;
	return strncat(d1, s1, n); /* adds null-character (n+1) */
}

void left_justify(char *dest, const char *source, size_t dest_size, char pad)
{
	bool copy = true;
	size_t i;
	for (i = 0; i < dest_size - 1; i++) {
		if (source[i] == 0) {
			copy = false;
		}
		if (copy) {
			dest[i] = source[i];
		} else {
			dest[i] = pad;
		}
	}
	dest[i] = 0;
}
