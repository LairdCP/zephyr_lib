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

#define RPMSG_LENGTH_BL5340PA_GET_VARIANT 1
#define RPMSG_LENGTH_BL5340PA_GET_ANTENNA 1
#define RPMSG_LENGTH_BL5340PA_SET_ANTENNA 2

#define RPMSG_LENGTH_BL5340PA_GET_VARIANT_RESPONSE 2
#define RPMSG_LENGTH_BL5340PA_GET_ANTENNA_RESPONSE 2
#define RPMSG_LENGTH_BL5340PA_SET_ANTENNA_RESPONSE 2
#define RPMSG_LENGTH_BL5340PA_ERROR 1

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
	RPMSG_OPCODE_BL5340PA_GET_VARIANT = 0,
	RPMSG_OPCODE_BL5340PA_GET_ANTENNA,
	RPMSG_OPCODE_BL5340PA_SET_ANTENNA,

	/* Errors */
	RPMSG_OPCODE_BL5340PA_INTERNAL_ERROR = 0xfa,
	RPMSG_OPCODE_BL5340PA_IO_ERROR = 0xfb,
	RPMSG_OPCODE_BL5340PA_WRONG_MODULE = 0xfc,
	RPMSG_OPCODE_BL5340PA_INVALID_ANTENNA = 0xfd,
	RPMSG_OPCODE_BL5340PA_INVALID_LENGTH = 0xfe,
	RPMSG_OPCODE_BL5340PA_INVALID_OPCODE = 0xff
} bl5340pa_rpmsg_opcode_t;

#endif /* __BL5340PA_H__ */
