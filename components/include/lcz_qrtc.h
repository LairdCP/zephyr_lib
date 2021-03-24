/**
 * @file lcz_qrtc.h
 * @brief Quasi Real Time Clock that uses offset and system ticks.
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __QRTC_H__
#define __QRTC_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Set the epoch
 *
 * @param epoch in seconds from Jan 1, 1970
 *
 * @retval recomputed value for testing
 */
uint32_t lcz_qrtc_set_epoch(uint32_t epoch);

/**
 * @brief Set the epoch using time structure.
 *
 * @param pTm is a pointer to a time structure
 * @param offset_seconds is the offset in seconds from UTC that the time structure
 * contains.
 *
 * @note The cellular modem provides local time and an offset and it is much
 * easier to add the offset to the epoch than to adjust the time structure.
 *
 * @retval epoch for testing
 */
uint32_t lcz_qrtc_set_epoch_from_tm(struct tm *pTm, int32_t offset_seconds);

/**
 * @retval Seconds since Jan 1, 1970.
 */
uint32_t lcz_qrtc_get_epoch(void);

/**
 * @retval true if the epoch has been set, otherwise false
 */
bool lcz_qrtc_epoch_was_set(void);

/**
 * @brief When enabled this is periodically called by qrtc module.
 *
 * @note Override the weak implementation in application.
 *
 */
#if CONFIG_LCZ_QRTC_SYNC_INTERVAL_SECONDS != 0
void lcz_qrtc_sync_handler(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __QRTC_H__ */
