/**
 * @file lcz_memfault_file.c
 * @brief Save Memfault data to file
 *
 * Copyright (c) 2021-2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lcz_mflt_file, CONFIG_LCZ_MEMFAULT_LOG_LEVEL);

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <zephyr/zephyr.h>
#include <zephyr/fs/fs.h>
#include <file_system_utilities.h>

#include "lcz_memfault.h"

/**************************************************************************************************/
/* Local Constant, Macro and Type Definitions                                                     */
/**************************************************************************************************/
#define CHUNK_HEADER_SIZE 2

/**************************************************************************************************/
/* Local Function Definitions                                                                     */
/**************************************************************************************************/
static int lcz_memfault_save_chunk_to_file(const char *abs_path, void *buf, uint16_t chunk_len)
{
	int append_status;

	LOG_DBG("Write chunk size %d", chunk_len);
	/* Each chunk needs to be framed so the consumer of the
	 * file can divide the chunks.
	 * Write two byte length header to frame the chunk.
	 */
	append_status = fsu_append_abs(abs_path, &chunk_len, sizeof(chunk_len));
	if (append_status < 0) {
		goto done;
	}
	/* write chunk */
	append_status = fsu_append_abs(abs_path, buf, chunk_len);
done:
	return append_status;
}

/**************************************************************************************************/
/* Global Function Definitions                                                                    */
/**************************************************************************************************/
int lcz_memfault_save_data_to_file(const char *abs_path, void *buf, size_t buf_size,
				   bool delete_file, bool save_coredump, size_t *file_size,
				   bool *has_core_dump)
{
	size_t chunk_len;
	bool data_available;
	size_t coredump_size = 0;
	int append_status = 0;

	*file_size = 0;
	*has_core_dump = memfault_coredump_has_valid_coredump(&coredump_size);

	/* Ignore error when file doesn't exist. */
	if (delete_file) {
		(void)fsu_delete_abs(abs_path);
	}

	if (!save_coredump) {
		memfault_packetizer_set_active_sources(kMfltDataSourceMask_Event |
						       kMfltDataSourceMask_Log);
		*has_core_dump = false;
	}

	do {
		chunk_len = buf_size;
		data_available = memfault_packetizer_get_chunk(buf, &chunk_len);
		if (data_available) {
			append_status =
				lcz_memfault_save_chunk_to_file(abs_path, buf, (uint16_t)chunk_len);
			*file_size += chunk_len + CHUNK_HEADER_SIZE;
		}
	} while (data_available && append_status >= 0);

	if (append_status < 0) {
		LOG_ERR("Memfault append to %s failed", abs_path);
	}
	LOG_DBG("file size: %d", *file_size);
	LOG_DBG("has coredump: %d", *has_core_dump);

	return append_status < 0 ? append_status : 0;
}
