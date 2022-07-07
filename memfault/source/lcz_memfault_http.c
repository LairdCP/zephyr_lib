/**
 * @file lcz_memfault_http.c
 * @brief Memfault HTTP integration
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_mflt_http, CONFIG_LCZ_MEMFAULT_LOG_LEVEL);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr.h>
#include "lcz_memfault.h"
#include "memfault/ports/zephyr/http.h"

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static K_MUTEX_DEFINE(post_data_mutex);

static int post_chunks(sMemfaultHttpContext *ctx, void *p_data, size_t *buf_size);

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static int post_chunks(sMemfaultHttpContext *ctx, void *p_data, size_t *buf_size)
{
	int rc = 0;
	size_t buf_len;

	buf_len = *buf_size;
	while (memfault_packetizer_get_chunk(p_data, buf_size)) {
		rc = memfault_zephyr_port_http_post_chunk(ctx, p_data, *buf_size);
		if (rc < 0) {
			LOG_ERR("Could not post chunk [%d]", rc);
			goto done;
		}
		*buf_size = buf_len;
	}
done:
	return rc;
}

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/

int lcz_memfault_http_init(void)
{
	int rc;

	rc = memfault_zephyr_port_install_root_certs();
	if (rc != 0) {
		LOG_ERR("Could not install memfault certs %d", rc);
	}

	return rc;
}

int lcz_memfault_post_data(void)
{
	int rc;

	k_mutex_lock(&post_data_mutex, K_FOREVER);
	rc = memfault_zephyr_port_post_data();
	k_mutex_unlock(&post_data_mutex);

	return rc;
}

int lcz_memfault_post_data_v2(void *buf, size_t buf_size)
{
	int rc;
	sMemfaultHttpContext http_ctx = { 0 };
	size_t data_len;

	rc = 0;
	http_ctx.sock_fd = -1;
	data_len = buf_size;

	k_mutex_lock(&post_data_mutex, K_FOREVER);

	if (!memfault_packetizer_data_available()) {
		goto exit;
	}

	rc = memfault_zephyr_port_http_open_socket(&http_ctx);
	if (rc < 0) {
		goto exit;
	}

	LOG_DBG("Sending memfault data...");
	rc = post_chunks(&http_ctx, buf, &data_len);
	if (rc < 0) {
		goto close;
	}
	LOG_DBG("Memfault data sent!");

close:
	memfault_zephyr_port_http_close_socket(&http_ctx);
exit:
	k_mutex_unlock(&post_data_mutex);
	return rc;
}
