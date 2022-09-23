/**
 * @file lcz_snprintk.c
 * @brief
 *
 * Copyright (c) 2022 Laird Connectivity LLC
 *
 * SPDX-License-Identifier: LicenseRef-LairdConnectivity-Clause
 */
#include <logging/log.h>
LOG_MODULE_REGISTER(snprintk, CONFIG_LCZ_SNPRINTK);

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <zephyr.h>
#include <stdio.h>

#include "lcz_snprintk.h"

/**************************************************************************************************/
/* Global Function Definitions                                                                    */
/**************************************************************************************************/
int lcz_snprintk(char *msg, size_t size, const char *fmt, ...)
{
	va_list ap;
	int actual_len;
	int r;

	va_start(ap, fmt);
	actual_len = vsnprintk(msg, size, fmt, ap);
	/* Has the string has been completely written? */
	if (actual_len > 0 && actual_len < size) {
		r = actual_len;
	} else {
		LOG_ERR("Unable to format message; actual len: %d string size: %d", actual_len,
			size);
		r = -EINVAL;
	}
	va_end(ap);

	return r;
}

int lcz_snprintk_malloc(char **msg, int *length, const char *fmt, ...)
{
	va_list ap;
	int actual_len = -1;
	int req_size;
	int r;
	char *buf = NULL;

	va_start(ap, fmt);
	do {
		/* Determine size of message */
		req_size = vsnprintk(buf, 0, fmt, ap);
		if (req_size < 0) {
			LOG_ERR("Invalid format or arguments");
			r = -EINVAL;
			break;
		}
		/* Add one for null character */
		req_size += 1;

		if (CONFIG_HEAP_MEM_POOL_SIZE > 0) {
			buf = k_calloc(req_size, sizeof(char));
		}

		if (buf == NULL) {
			LOG_ERR("Unable to allocate message");
			r = -ENOMEM;
			break;
		} else {
			LOG_DBG("Message size %d", req_size);
		}

		/* Build message and determine if string has been completely written. */
		actual_len = vsnprintk(buf, req_size, fmt, ap);
		if (actual_len > 0 && actual_len < req_size) {
			r = actual_len;
		} else {
			LOG_ERR("Unable to format message; actual len: %d string size: %d",
				actual_len, req_size);
			r = -EINVAL;
		}

	} while (0);

	va_end(ap);
	if (length != NULL) {
		*length = actual_len;
	}
	*msg = buf;
	return r;
}