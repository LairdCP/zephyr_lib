/**
 * @file ad_find.h
 * @brief Find TLV (type, length, value) structures in advertisements.
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __AD_FIND_H__
#define __AD_FIND_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Constants, Macros and Type Definitions                              */
/******************************************************************************/
#define BT_DATA_INVALID 0x00

typedef struct AdHandle {
	uint8_t *pPayload;
	size_t size;
} AdHandle_t;

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Finds a TLV in advertisement
 *
 * @param pAdv pointer to advertisement data
 * @param Length length of the data
 * @param Type1 type of TLV to find
 * @param Type2 second type of tlv to find, set to BT_DATA_INVALID when not used.
 * Parsing will stop on first type found.
 *
 * @retval AdHandle_t - pointer to payload if found otherwise NULL
 */
AdHandle_t AdFind_Type(uint8_t *pAdv, size_t Length, uint8_t Type1, uint8_t Type2);

/**
 * @brief Finds a short or complete name in advertisement.
 *
 * @retval AdHandle_t pointer to payload if found otherwise NULL
 */
AdHandle_t AdFind_Name(uint8_t *pAdv, size_t Length);

/**
 * @brief Compares name string with name field in advertisement.
 *
 * @retval true if name field is in advertisement and NameLength bytes match,
 * otherwise false
 */
bool AdFind_MatchName(uint8_t *pAdv, size_t Length, char *Name, size_t NameLength);

#ifdef __cplusplus
}
#endif

#endif /* __AD_FIND_H__ */
