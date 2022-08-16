/**
 * @file lcz_sensor_adv_match.c
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include "lcz_bluetooth.h"
#include "lcz_sensor_adv_format.h"
#include "lcz_sensor_adv_match.h"

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
uint16_t lcz_sensor_adv_match(struct net_buf_simple *ad, bool match_rsp,
			      bool match_coded)
{
	AdHandle_t handle = AdFind_Type(
		ad->data, ad->len, BT_DATA_MANUFACTURER_DATA, BT_DATA_INVALID);

	if (handle.pPayload == NULL) {
		return RESERVED_AD_PROTOCOL_ID;
	}

	if (lcz_sensor_adv_match_1m(&handle)) {
		return BTXXX_1M_PHY_AD_PROTOCOL_ID;
	}

	if (match_rsp) {
		if (lcz_sensor_adv_match_rsp(&handle)) {
			return BTXXX_1M_PHY_RSP_PROTOCOL_ID;
		}
	}

	if (match_coded) {
		if (lcz_sensor_adv_match_coded(&handle)) {
			return BTXXX_CODED_PHY_AD_PROTOCOL_ID;
		}
	}

	return RESERVED_AD_PROTOCOL_ID;
}

/* The BT510 and BT6xx advertisement can be recognized by the manufacturer
 * specific data type with Laird Connectivity as the company ID.
 * It is further qualified by having a length of 27 and matching protocol ID.
 * BT710 (Contact Tracing) has a different protocol ID and will not be matched.
 */
bool lcz_sensor_adv_match_1m(AdHandle_t *handle)
{
	if (handle->pPayload != NULL) {
		if ((handle->size == LCZ_SENSOR_MSD_AD_PAYLOAD_LENGTH)) {
			if (memcmp(handle->pPayload, BTXXX_AD_HEADER,
				   sizeof(BTXXX_AD_HEADER)) == 0) {
				return true;
			}
		}
	}
	return false;
}

bool lcz_sensor_adv_match_rsp(AdHandle_t *handle)
{
	if (handle->pPayload != NULL) {
		if ((handle->size == LCZ_SENSOR_MSD_RSP_PAYLOAD_LENGTH)) {
			if (memcmp(handle->pPayload, BT5XX_RSP_HEADER,
				   sizeof(BT5XX_RSP_HEADER)) == 0) {
				return true;
			}
			if (memcmp(handle->pPayload, BT6XX_RSP_HEADER,
				   sizeof(BT6XX_RSP_HEADER)) == 0) {
				return true;
			}
		}
	}
	return false;
}

bool lcz_sensor_adv_match_coded(AdHandle_t *handle)
{
	if (handle->pPayload != NULL) {
		if ((handle->size == LCZ_SENSOR_MSD_CODED_PAYLOAD_LENGTH)) {
			if (memcmp(handle->pPayload, BTXXX_CODED_HEADER,
				   sizeof(BTXXX_CODED_HEADER)) == 0) {
				return true;
			}
		}
	}
	return false;
}
