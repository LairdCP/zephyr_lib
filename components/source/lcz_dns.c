/**
 * @file lcz_dns.c
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(dns);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <kernel.h>
#include <stdio.h>
#include <net/dns_resolve.h>

#include "lcz_dns.h"

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
int dns_resolve_server_addr(char *endpoint, char *port, struct addrinfo *hints,
			    struct addrinfo **result)
{
	int rc = -EPERM;
	int dns_retries = CONFIG_DNS_RETRIES;
	do {
		rc = getaddrinfo(endpoint, port, hints, result);
		if (rc != 0) {
			LOG_ERR("Get server addr (%d)", rc);
			k_sleep(K_SECONDS(CONFIG_DNS_RETRY_DELAY_SECONDS));
		}
		dns_retries--;
	} while (rc != 0 && dns_retries != 0);

	if (rc != 0) {
		LOG_ERR("Unable to resolve '%s'", log_strdup(endpoint));
	}
	return rc;
}

int dns_build_addr_string(char *server_addr, struct addrinfo *result)
{
	int rc = -EPROTONOSUPPORT;
	memset(server_addr, 0, CONFIG_DNS_RESOLVER_ADDR_MAX_SIZE);
	LOG_DBG("Address Family %u", result->ai_family);
	if (result->ai_family == AF_INET6) {
		snprintk(server_addr, CONFIG_DNS_RESOLVER_ADDR_MAX_SIZE,
			 "%u.%u.%u.%u.%u.%u", result->ai_addr->data[0],
			 result->ai_addr->data[1], result->ai_addr->data[2],
			 result->ai_addr->data[3], result->ai_addr->data[4],
			 result->ai_addr->data[5]);
		rc = 0;
	} else if (result->ai_family == AF_INET) {
		snprintk(server_addr, CONFIG_DNS_RESOLVER_ADDR_MAX_SIZE,
			 "%u.%u.%u.%u", result->ai_addr->data[2],
			 result->ai_addr->data[3], result->ai_addr->data[4],
			 result->ai_addr->data[5]);
		rc = 0;
	} else {
		LOG_ERR("DNS result family is %u", result->ai_family);
	}
	return rc;
}