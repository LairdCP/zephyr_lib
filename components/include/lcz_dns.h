/**
 * @file lcz_dns.h
 * @brief Wrapper for DNS functions.
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_DNS_H__
#define __LCZ_DNS_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief Wraps getaddrinfo with DNS_RETRIES counter and print statements.
 */
int dns_resolve_server_addr(char *endpoint, char *port, struct addrinfo *hints,
			    struct addrinfo **result);

/**
 * @brief Convert getaddrinfo result into address string using snprintk.
 *
 * @param server_addr is a string at least CONFIG_DNS_RESOLVER_ADDR_MAX_SIZE
 * long
 * @param result is the result of dns_resolve_server_addr
 *
 * @retval 0 on success.
 */
int dns_build_addr_string(char *server_addr, struct addrinfo *result);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_DNS_H__ */
