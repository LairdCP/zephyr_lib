/**
 * @file laird_connectivity_nfc.h
 * @brief Laird NFC Support.
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_NFC_H__
#define __LCZ_NFC_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Constants, Macros and Type Definitions                              */
/******************************************************************************/
#define MAX_REC_COUNT		2
#define MAX_REC_PAYLOAD		96

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/** @brief Initializes the NFC subsystem.
 */
int laird_connectivity_nfc_init();

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_NFC_H__ */
