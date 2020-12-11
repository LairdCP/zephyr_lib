/**
 * @file lcz_no_init_ram_var.c
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <sys/crc.h>

#include "lcz_no_init_ram_var.h"

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
bool lcz_no_init_ram_var_is_valid(void *data, size_t size)
{
	no_init_ram_header_t *hdr = (no_init_ram_header_t *)data;
	uint32_t length = size + NO_INIT_RAM_HEADER_SIZE_WITHOUT_CRC;
	uint32_t crc = 0;

	if (hdr->key != CONFIG_LCZ_NO_INIT_RAM_VAR_VALID_KEY) {
		return false;
	}

	if (hdr->size != size) {
		return false;
	}

	crc = crc32_ieee((uint8_t *)&hdr->key, length);
	return (crc == hdr->crc);
}

void lcz_no_init_ram_var_update_header(void *data, size_t size)
{
	uint32_t length = size + NO_INIT_RAM_HEADER_SIZE_WITHOUT_CRC;
	no_init_ram_header_t *hdr = (no_init_ram_header_t *)data;

	hdr->key = CONFIG_LCZ_NO_INIT_RAM_VAR_VALID_KEY;
	hdr->size = size;
	hdr->crc = crc32_ieee((uint8_t *)&hdr->key, length);
}
