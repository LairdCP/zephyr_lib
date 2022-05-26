/**
 * @file lcz_memfault_log.c
 * @brief
 *
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_mflt_log, CONFIG_LCZ_MEMFAULT_LOG_LEVEL);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr.h>

#include "file_system_utilities.h"

#include "memfault/core/log.h"

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
int lcz_memfault_log_save(const char *file_name)
{
	int r = 0;
	sMemfaultLog log = { 0 };
	size_t size = 0;
	size_t count = 0;

	r = fsu_delete_abs(file_name);
	LOG_INF("Delete previous log: %d", r);

	LOG_WRN("Writing log. Please wait...");

	while (memfault_log_read(&log)) {
		if (log.type == kMemfaultLogRecordType_Preformatted) {
			if (log.msg_len > 0) {
				if (fsu_append_abs(file_name, log.msg,
						   log.msg_len) >= 0) {
					size += log.msg_len;
					count += 1;
				} else {
					r = -EIO;
					LOG_ERR("Gateway log append error");
					break;
				}
			} else {
				r = -EINVAL;
				LOG_ERR("Log message length of zero");
				break;
			}
		} else {
			r = -EINVAL;
			LOG_ERR("Unexpected log format");
			break;
		}
	}

	/* Allow file to be treated like a string */
	if (size > 0) {
		fsu_append_abs(file_name, 0, 1);
	}

	LOG_INF("%s: lines: %u size: %u", file_name, count, size);

	return r;
}
