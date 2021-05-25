/**
 * @file lcz_bracket.h
 * @brief Handles bracket matching on a JSON serial stream.
 * A packet framer and rudimentary parser.
 *
 * Copyright (c) 2018-2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_BRACKET_H__
#define __LCZ_BRACKET_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <kernel.h>
#include <stddef.h>
#include <zephyr/types.h>

/******************************************************************************/
/* Global Constants, Macros and Type Definitions                              */
/******************************************************************************/
#define BRACKET_MATCH 0
#define BRACKET_MATCHING_NOT_STARTED 1

typedef struct bracket bracket_t;

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Initializes bracket handler.
 *
 * @param size of the buffer
 * @param p is a pointer to buffer used for object.  The buffer must be aligned
 * to a 4-byte boundary.
 *
 * @retval pointer to bracket object, NULL if there was an error.
 */
bracket_t *lcz_bracket_initialize(size_t size, void *p);

/**
 * @brief Resets bracket (delta and index).
 *
 * @param p is a pointer to bracket object
 */
void lcz_bracket_reset(bracket_t *p);

/**
 * @brief Accessor Function
 *
 * @param p is a pointer to bracket object
 *
 * @retval true if bracket delta is 0, false otherwise
 */
bool lcz_bracket_match(bracket_t *p);

/**
 * @brief Accessor Function
 *
 * @param p is a pointer to bracket object
 *
 * @retval true if the first '{' has been found after bracket was reset.
 */
bool lcz_bracket_entered(bracket_t *p);

/**
 * @brief Accessor Function
 *
 * @param p is a pointer to bracket object
 *
 * @retval the number of used bytes in the bracket.
 */
size_t lcz_bracket_length(bracket_t *p);

/**
 * @brief Accessor Function
 *
 * @param p is a pointer to bracket object
 *
 * @retval the total number of bytes in the bracket.
 */
size_t lcz_bracket_size(bracket_t *p);

/**
 * @brief Counts the number of { } matching pairs in a JSON stream.
 * If the first bracket has been found then the character is added to the
 * buffer.
 *
 * @param p is a pointer to bracket object
 * @param character is the next character to process.
 *
 * @retval 0 for bracket match (at least one '{' was found).
 * @retval > 0 the delta between { and }. This will be 1 if first '{'
 * hasn't been found.
 * @retval -ENOMEM if there wasn't enough memory to process incoming stream
 */
int lcz_bracket_compute(bracket_t *p, char character);

/**
 * @brief Copies the current buffer into the destination (dest) if non-NULL.
 *
 * @param dest if NULL, then nothing is copied but length is still valid.
 *
 * @retval The number of bytes in the buffer (length).
 */
size_t lcz_bracket_copy(bracket_t *p, void *dest);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_BRACKET_H__ */
