/**
 * @file lcz_sensor_adv_match.h
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_SENSOR_ADV_MATCH__
#define __LCZ_SENSOR_ADV_MATCH__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>
#include <stddef.h>
#include <bluetooth/bluetooth.h>

#include "ad_find.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/

/**
 * @brief Parse advertisement to determine if it matches BT510 or BT6xx format.
 *
 * @param ad buffer from scan callback
 * @param match_rsp enable scan response matching
 * @param match_coded enabled coded PHY matching
 * @return uint16_t RESERVED_AD_PROTOCOL_ID (0) if ad doesn't match,
 * AD_PROTOCOL_ID otherwise.
 *
 */
uint16_t lcz_sensor_adv_match(struct net_buf_simple *ad, bool match_rsp,
			      bool match_coded);

/**
 * @brief Match BT510 or BT6xx 1M advertisement
 *
 * @param handle payload of manufacturer specific ad
 * @return true if match, false otherwise
 */
bool lcz_sensor_adv_match_1m(AdHandle_t *handle);

/**
 * @brief Match BT510 or BT6xx scan response
 *
 * @param handle payload of manufacturer specific ad
 * @return true if match, false otherwise
 */
bool lcz_sensor_adv_match_rsp(AdHandle_t *handle);

/**
 * @brief Match BT510 or BT6xx coded PHY ad
 *
 * @param handle payload of manufacturer specific ad
 * @return true if match, false otherwise
 */
bool lcz_sensor_adv_match_coded(AdHandle_t *handle);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_SENSOR_ADV_MATCH__ */
