/**
 * @file lcz_no_init_ram_var.h
 *
 * @brief Helper for working with non-intialized variables stored in RAM.
 * Each piece of data contains a key and CRC for determining if data is most
 * likely valid.
 *
 * @example (no bootloader or
 * noinit sections line up between bootloader and app)
 *
 * static no_init_ram_uint32_t foo __attribute__((section(".noinit")));
 *
 * @example (mcuboot with partition manager)
 * static no_init_ram_uint32_t *bar =
 *	(no_init_ram_uint32_t *)PM_LCZ_NOINIT_SRAM_ADDRESS;
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_NO_INIT_RAM_VAR_H__
#define __LCZ_NO_INIT_RAM_VAR_H__

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
#define NO_INIT_RAM_HEADER_SIZE 12
#define NO_INIT_RAM_HEADER_SIZE_WITHOUT_CRC 8

/* Size is much larger than required but keeps data 32-bit aligned. */
typedef struct no_init_ram_header {
	uint32_t crc;
	uint32_t key;
	uint32_t size;
} no_init_ram_header_t;

/*
 * A commonly used case is a 32-bit quantity. However, any size can be used.
 * A CRC32 is computed on the data so the data may need to be packed.
 * {
 *     no_init_ram_header_t header;
 *     uint8_t foo[16];
 *     uint8_t bar[2];
 * }
 */
typedef struct no_init_ram_uint32 {
	no_init_ram_header_t header;
	uint32_t data;
} no_init_ram_uint32_t;

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Used to determine if a value in non-initialized ram is most likely
 * valid.
 *
 * @param data - A pointer to a structure that contains the header and data.
 * @param size - size of the data in bytes
 *
 * @retval true if the header is valid, false otherwise.
 */
bool lcz_no_init_ram_var_is_valid(void *data, size_t size);

/**
 * @brief  Updates the header for a non-initialized RAM variable
 * Assumes data was already set.
 *
 * @param data - A pointer to a structure that contains the header and data.
 * @param size - size of the data in bytes
 *
 */
void lcz_no_init_ram_var_update_header(void *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_NO_INIT_RAM_VAR_H__ */
