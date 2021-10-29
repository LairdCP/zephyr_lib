/**
 * @file bl5340pa.h
 * @brief Component for getting/setting BL5340PA specific functionality
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BL5340PA_H__
#define __BL5340PA_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>
#include <stddef.h>

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define RPMSG_COMPONENT_BL5340PA 0

#define RPMSG_LENGTH_BL5340PA_GET_STATUS 1
#define RPMSG_LENGTH_BL5340PA_GET_VARIANT 1
#define RPMSG_LENGTH_BL5340PA_GET_ANTENNA 1
#define RPMSG_LENGTH_BL5340PA_SET_ANTENNA 2

#define RPMSG_LENGTH_BL5340PA_GET_STATUS_RESPONSE 8
#define RPMSG_LENGTH_BL5340PA_GET_VARIANT_RESPONSE 2
#define RPMSG_LENGTH_BL5340PA_GET_ANTENNA_RESPONSE 2
#define RPMSG_LENGTH_BL5340PA_SET_ANTENNA_RESPONSE 2
#define RPMSG_LENGTH_BL5340PA_ERROR 1
#define RPMSG_LENGTH_BL5340PA_REGULATORY_ERROR 7

typedef enum {
	RPMSG_BL5340PA_OFFSET_OPCODE = 0,
	RPMSG_BL5340PA_OFFSET_DATA = 1
} bl5340pa_rpmsg_offset_t;

typedef enum {
	BL5340PA_ANTENNA_PIN_INTERNAL_ANTENNA = 0,
	BL5340PA_ANTENNA_PIN_EXTERNAL_ANTENNA = 1,

	BL5340PA_ANTENNA_PIN_COUNT
} bl5340pa_antenna_pin_t;

typedef enum {
	BL5340PA_VARIANT_INTERNAL_ANTENNA = 0,
	BL5340PA_VARIANT_EXTERNAL_ANTENNA = 1,

	BL5340PA_VARIANT_COUNT
} bl5340pa_variant_t;

typedef enum {
	BL5340PA_RADIO_STATUS_NOT_KNOWN = 0,
	BL5340PA_RADIO_STATUS_ENABLED,
	BL5340PA_RADIO_STATUS_DISABLED,

	BL5340PA_RADIO_STATUS_COUNT
} bl5340pa_radio_status_t;

typedef enum {
	BL5340PA_RADIO_TYPE_BLE = 0,
	BL5340PA_RADIO_TYPE_802_15_4,

	BL5340PA_RADIO_TYPE_COUNT
} bl5340pa_radio_type_t;

typedef enum {
	RPMSG_OPCODE_BL5340PA_GET_STATUS = 0,
	RPMSG_OPCODE_BL5340PA_GET_VARIANT,
	RPMSG_OPCODE_BL5340PA_GET_ANTENNA,
	RPMSG_OPCODE_BL5340PA_SET_ANTENNA,

	/* Errors */
	RPMSG_OPCODE_BL5340PA_REGULATORY_ERROR = 0xf9,
	RPMSG_OPCODE_BL5340PA_INTERNAL_ERROR = 0xfa,
	RPMSG_OPCODE_BL5340PA_IO_ERROR = 0xfb,
	RPMSG_OPCODE_BL5340PA_WRONG_MODULE = 0xfc,
	RPMSG_OPCODE_BL5340PA_INVALID_ANTENNA = 0xfd,
	RPMSG_OPCODE_BL5340PA_INVALID_LENGTH = 0xfe,
	RPMSG_OPCODE_BL5340PA_INVALID_OPCODE = 0xff
} bl5340pa_rpmsg_opcode_t;

typedef enum {
	BL5340PA_STATUS_OFFSET_STATUS = 1,
	BL5340PA_STATUS_OFFSET_STACK = 2,
	BL5340PA_STATUS_OFFSET_OPTION = 3,
	BL5340PA_STATUS_OFFSET_ERROR = 4
} bl5340pa_status_offset_t;

typedef enum {
	BL5340PA_STATUS_LENGTH_STATUS = 1,
	BL5340PA_STATUS_LENGTH_STACK = 1,
	BL5340PA_STATUS_LENGTH_OPTION = 1,
	BL5340PA_STATUS_LENGTH_ERROR = 4
} bl5340pa_status_length_t;

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
#if defined(CONFIG_LIMIT_RADIO_POWER)
/**
 * @brief Report error (or success) of radio power limitations being applied
 *
 * @param type The type of stack being initialised (bl5340pa_radio_type_t)
 * @param option The option which failed, for a BLE stack this is the PHY
 * @param error 0 for no error (success) otherwise error code
 */
void bl5340pa_regulatory_error(uint8_t type, uint8_t option, int32_t error);
#endif

#endif /* __BL5340PA_H__ */
