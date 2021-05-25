/**
 * @file lcz_bracket.c
 * @brief
 *
 * Copyright (c) 2018-2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_bracket, CONFIG_LOG_LEVEL_LCZ_BRACKET);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "lcz_bracket.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
struct bracket {
	size_t size;
	size_t length;
	int32_t delta;
	bool entered;
	char buffer[];
};

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
bracket_t *lcz_bracket_initialize(size_t size, void *p)
{
	__ASSERT(p != NULL,
		 "Attempt to initialize lcz_bracket with NULL pointer");
	__ASSERT((POINTER_TO_UINT(p) & 3) == 0,
		 "Buffer must be aligned to a 4-byte boundary");
	__ASSERT(
		size > sizeof(bracket_t),
		"Attempt to initialize lcz_bracket with too small of a buffer");

	if ((size > sizeof(bracket_t)) && (p != NULL) &&
	    ((POINTER_TO_UINT(p) & 3) == 0)) {
		memset(p, 0, size);
		size_t s = size - sizeof(bracket_t);
		bracket_t *ptr = (bracket_t *)p;
		lcz_bracket_reset(ptr);
		ptr->size = s;
		return ptr;
	} else {
		return NULL;
	}
}

void lcz_bracket_reset(bracket_t *p)
{
	p->entered = false;
	p->length = 0;
	p->delta = 0;
}

bool lcz_bracket_match(bracket_t *p)
{
	return ((p->delta == 0) && p->entered);
}

bool lcz_bracket_entered(bracket_t *p)
{
	return p->entered;
}

size_t lcz_bracket_length(bracket_t *p)
{
	return p->length;
}

size_t lcz_bracket_size(bracket_t *p)
{
	return p->size;
}

int lcz_bracket_compute(bracket_t *p, char character)
{
	if (character == '{') {
		p->delta++;
		p->entered = true;
	}

	/* The LAIRD dongle in vSP mode sends status at the start and end of a
	 * connection (~300 bytes). If present, ignore that data.
	 */
	if (p->entered) {
		if (character == '}') {
			p->delta--;
		}

		p->buffer[p->length++] = character;

#ifdef CONFIG_LCZ_BRACKET_INTERCEPTS_ESCAPED_FORWARD_SLASH
		if (p->length >= 2) {
			if (p->buffer[p->length - 2] == '\\' &&
			    p->buffer[p->length - 1] == '/') {
				p->buffer[p->length - 2] = '/';
				p->length -= 1;
			}
		}
#endif
	}

	if (lcz_bracket_match(p)) {
		return BRACKET_MATCH;
	} else if (p->length == p->size) {
		LOG_ERR("Not enough memory for lcz_bracket handler");
		lcz_bracket_reset(p);
		return -ENOMEM;
	} else if (p->entered) {
		return abs(p->delta);
	} else {
		return BRACKET_MATCHING_NOT_STARTED;
	}
}

size_t lcz_bracket_copy(bracket_t *p, void *dest)
{
	if (dest != NULL) {
		memcpy(dest, p->buffer, p->length);
	}
	return p->length;
}
