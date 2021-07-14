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
#include "memfault/http/http_client.h"
#include "memfault/nrfconnect_port/http.h"

/******************************************************************************/
/* Global Data Definitions                                                    */
/******************************************************************************/
K_MUTEX_DEFINE(post_data_mutex);

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
