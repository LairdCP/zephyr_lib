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
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define WORKQ_SIZE CONFIG_LCZ_MEMFAULT_HTTP_PUBLISH_WORKQ_SIZE

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
#if WORKQ_SIZE != 0
static void memfault_http_work_handler(struct k_work *work);
#endif

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
#if WORKQ_SIZE != 0
static struct k_work_q memfault_http_work_q;
static K_THREAD_STACK_DEFINE(memfault_http_stack, WORKQ_SIZE);
static K_WORK_DEFINE(memfault_http_work, memfault_http_work_handler);
#else
static K_MUTEX_DEFINE(post_data_mutex);
#endif

static bool memfault_post_busy;

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

#if WORKQ_SIZE != 0
	struct k_work_queue_config cfg = {
		.name = "mflt_http",
	};

	k_work_queue_init(&memfault_http_work_q);

	k_work_queue_start(&memfault_http_work_q, memfault_http_stack,
			   K_THREAD_STACK_SIZEOF(memfault_http_stack),
			   (CONFIG_NUM_PREEMPT_PRIORITIES - 1), &cfg);
#endif

	return rc;
}

int lcz_memfault_post_data(void)
{
	int rc;

	memfault_post_busy = true;

#if WORKQ_SIZE != 0
	rc = k_work_submit_to_queue(&memfault_http_work_q, &memfault_http_work);
#else
	k_mutex_lock(&post_data_mutex, K_FOREVER);
	rc = memfault_zephyr_port_post_data();
	k_mutex_unlock(&post_data_mutex);
	memfault_post_busy = false;
#endif

	return rc;
}

bool lcz_memfault_post_busy(void)
{
	return memfault_post_busy;
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
#if WORKQ_SIZE != 0
static void memfault_http_work_handler(struct k_work *work)
{
	int r;

	r = memfault_zephyr_port_post_data();
	LOG_INF("Memfault post status: %d", r);

	memfault_post_busy = false;
}
#endif
