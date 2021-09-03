/**
 * @file lcz_memfault_file.c
 * @brief Save Memfault data to file
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_mflt_file, CONFIG_LCZ_MEMFAULT_LOG_LEVEL);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr.h>
#include <fs/fs.h>

#include "file_system_utilities.h"
#include "lcz_memfault.h"

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/

static uint8_t chunk[CONFIG_LCZ_MEMFAULT_FILE_CHUNK_SIZE];

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/

int lcz_memfault_save_data_to_file(const char *abs_path, size_t *file_size,
				   bool *has_core_dump)
{
	size_t chunk_len;
	uint16_t chunk_header;
	bool data_available;
	size_t coredump_size = 0;
	int append_status = 0;

	*file_size = 0;
	*has_core_dump = memfault_coredump_has_valid_coredump(&coredump_size);

	/* Ignore error when file doesn't exist. */
	(void)fsu_delete_abs(abs_path);

	do {
		chunk_len = sizeof(chunk);
		data_available =
			memfault_packetizer_get_chunk(chunk, &chunk_len);
		if (data_available) {
			LOG_DBG("Write chunk size %d", chunk_len);
			chunk_header = chunk_len;
			/* Each chunk needs to be framed so the consumer of the
			 * file can divide the chunks.
			 * Write two byte length header to frame the chunk. */
			append_status = fsu_append_abs(abs_path, &chunk_header,
						       sizeof(chunk_header));
			/* write chunk */
			append_status =
				fsu_append_abs(abs_path, chunk, chunk_len);
			*file_size += chunk_len + sizeof(chunk_header);
		}
	} while (data_available && append_status >= 0);

	if (append_status < 0) {
		LOG_ERR("Memfault append to %s failed", abs_path);
	}
	LOG_DBG("file size: %d", *file_size);
	LOG_DBG("has coredump: %d", *has_core_dump);

	return append_status < 0 ? append_status : 0;
}
