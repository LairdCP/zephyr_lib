/**
 * @file lcz_memfault_coap.c
 * @brief Memfault COAP transport
 *
 * Copyright (c) 2021-2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lcz_mflt_coap, CONFIG_LCZ_MEMFAULT_LOG_LEVEL);

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <stdarg.h>
#include <zephyr/zephyr.h>
#include <zephyr/random/rand32.h>
#include <memfault/core/data_packetizer.h>
#include <attr.h>
#include <net/coap.h>
#include <net/socket.h>
#include <kernel.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <net/tls_credentials.h>
#include <mbedtls/ssl.h>

#include "lcz_memfault.h"
#include "lcz_sock.h"
#include "lcz_pki_auth.h"
#include "lcz_dns.h"
#include "lcz_coap_telemetry.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/

/**************************************************************************************************/
/* Local Data Definitions                                                                         */
/**************************************************************************************************/
static struct {
	bool initialized;
	bool busy;
	struct k_sem sem;

} coap_memfault;

static lcz_coap_telemetry_query_t query;

/**************************************************************************************************/
/* Local Function Prototypes                                                                      */
/**************************************************************************************************/
static void coap_memfault_init(void);
static int coap_memfault_send_data(char *buf, size_t buf_size, k_timeout_t chunk_timeout);

/**************************************************************************************************/
/* Global Function Definitions                                                                    */
/**************************************************************************************************/
int lcz_memfault_coap_publish_data(char *buf, size_t buf_size, k_timeout_t chunk_timeout)
{
	int r = -EAGAIN;

	if (!coap_memfault.busy) {
		coap_memfault.busy = true;
		r = coap_memfault_send_data(buf, buf_size, chunk_timeout);
		coap_memfault.busy = false;
	}

	return r;
}

bool lcz_memfault_coap_enabled(void)
{
	return attr_get_uint32(ATTR_ID_memfault_transport, 0) == MEMFAULT_TRANSPORT_COAP;
}

void lcz_memfault_coap_init(char *domain, bool dtls, uint16_t port, char *url_path, char *proxy,
				bool peer_verify, bool hostname_verify)
{
	query.domain = domain;
	query.dtls = dtls;
	query.port = port;
	query.path = url_path;
	snprintf(query.proxy_url, sizeof(query.proxy_url), "%s", proxy);
	query.peer_verify = peer_verify;
	query.hostname_verify = hostname_verify;
}

/**************************************************************************************************/
/* Local Function Definitions                                                                     */
/**************************************************************************************************/
static void coap_memfault_init(void)
{
	if (!coap_memfault.initialized) {
		k_sem_init(&coap_memfault.sem, 1, 1);

		coap_memfault.initialized = true;
	}
}

static int coap_memfault_send_data(char *buf, size_t buf_size, k_timeout_t chunk_timeout)
{
	int rc = 0;
	size_t data_len;
	bool data_available;
	
	if (attr_get_uint32(ATTR_ID_memfault_transport, 0) != MEMFAULT_TRANSPORT_COAP) {
		return -EPERM;
	}

	if (query.domain == NULL || strlen(query.domain) == 0) {
		LOG_ERR("CoAP Endpoint must be set to send data using COAP");
		return -EINVAL;
	}

	coap_memfault_init();

	LOG_DBG("Starting...");

	rc = k_sem_take(&coap_memfault.sem, chunk_timeout);
	if (rc != 0) {
		LOG_ERR("Memfault COAP timeout: Could not take semaphore %d", rc);
		return rc;
	}

	while (1) {
		data_len = buf_size;
		data_available = memfault_packetizer_get_chunk(buf, &data_len);
		if (!data_available) {
			LOG_DBG("No more data to send");
			break;
		}

		query.pData = buf;
		query.dataLen = data_len;
	  
		rc = lcz_coap_telemetry_post(&query);
		
		if (rc != 0) {
			LOG_ERR("Could not publish Memfault data %d", rc);
			break;
		} else {
			LOG_DBG("Sending %d bytes", data_len);
		}
	}

	LOG_INF("Done: %d", rc);

	k_sem_give(&coap_memfault.sem);

	return rc;
}
