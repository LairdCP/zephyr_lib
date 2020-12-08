/**
 * @file lcz_sensor_adv_format.c
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include "laird_bluetooth.h"
#include "lcz_sensor_adv_format.h"

/******************************************************************************/
/* Global Data Definitions                                                    */
/******************************************************************************/
const uint8_t BTXXX_AD_HEADER[LCZ_SENSOR_AD_HEADER_SIZE] = {
	LSB_16(LAIRD_CONNECTIVITY_MANUFACTURER_SPECIFIC_COMPANY_ID1),
	MSB_16(LAIRD_CONNECTIVITY_MANUFACTURER_SPECIFIC_COMPANY_ID1),
	LSB_16(BTXXX_1M_PHY_AD_PROTOCOL_ID), MSB_16(BTXXX_1M_PHY_AD_PROTOCOL_ID)
};

const uint8_t BTXXX_RSP_HEADER[LCZ_SENSOR_AD_HEADER_SIZE] = {
	LSB_16(LAIRD_CONNECTIVITY_MANUFACTURER_SPECIFIC_COMPANY_ID2),
	MSB_16(LAIRD_CONNECTIVITY_MANUFACTURER_SPECIFIC_COMPANY_ID2),
	LSB_16(BTXXX_1M_PHY_RSP_PROTOCOL_ID),
	MSB_16(BTXXX_1M_PHY_RSP_PROTOCOL_ID)
};

const uint8_t BTXXX_CODED_HEADER[LCZ_SENSOR_AD_HEADER_SIZE] = {
	LSB_16(LAIRD_CONNECTIVITY_MANUFACTURER_SPECIFIC_COMPANY_ID1),
	MSB_16(LAIRD_CONNECTIVITY_MANUFACTURER_SPECIFIC_COMPANY_ID1),
	LSB_16(BTXXX_CODED_PHY_AD_PROTOCOL_ID),
	MSB_16(BTXXX_CODED_PHY_AD_PROTOCOL_ID)
};