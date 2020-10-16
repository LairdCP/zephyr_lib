/**
 * @file lcz_sock.c
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(lcz_sock);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <string.h>

#include "lcz_sock.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define p_sock p->fds[0].fd

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static void lcz_sock_prepare_fds(sock_info_t *p);
static void lcz_sock_clear_fds(sock_info_t *p);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void lcz_sock_set_name(sock_info_t *p, const char *const name)
{
	if (p != NULL) {
		p->name = name;
	}
}

void lcz_sock_set_events(sock_info_t *p, int events)
{
	if (p != NULL) {
		p->fds[0].events = events;
	}
}

void lcz_sock_enable_dtls(sock_info_t *p, int (*load_credentials)(void))
{
	if (p != NULL) {
		p->use_dtls = true;
		p->load_credentials = load_credentials;
	}
}

void lcz_sock_disable_dtls(sock_info_t *p)
{
	if (p != NULL) {
		p->use_dtls = false;
	}
}

void lcz_sock_set_tls_tag_list(sock_info_t *p, const sec_tag_t *const list,
			       size_t list_size)
{
	if (p != NULL) {
		p->tls_tag_list = list;
		p->list_size = list_size;
	}
}

int lcz_get_sock(sock_info_t *p)
{
	if (p != NULL) {
		return p_sock;
	} else {
		return -1;
	}
}

bool lcz_sock_valid(sock_info_t *p)
{
	if (p == NULL) {
		return false;
	} else {
		return (p->nfds != 0);
	}
}

int lcz_sock_wait(sock_info_t *p, int timeout)
{
	if (p == NULL) {
		LOG_DBG("Invalid parameters");
		return -EINVAL;
	}

	int r = -EPERM;
	if (p->nfds > 0) {
		r = poll(p->fds, p->nfds, timeout);
		if (r < 0) {
			LOG_ERR("%s Poll Error: -%d", log_strdup(p->name),
				errno);
			r = -errno;
		} else if (r == 0) {
			if (IS_ENABLED(CONFIG_LCZ_SOCK_VERBOSE_POLL)) {
				LOG_DBG("%s Poll Timeout", log_strdup(p->name));
			}
			r = -ETIME;
		} else {
			if (IS_ENABLED(CONFIG_LCZ_SOCK_VERBOSE_POLL)) {
				LOG_DBG("Wait complete");
			}
			r = 0;
		}
	} else {
		LOG_ERR("Sock not valid");
	}
	return r;
}

int lcz_udp_sock_start(sock_info_t *p, struct sockaddr *addr, const char *name)
{
	if (p == NULL || addr == NULL) {
		LOG_DBG("Invalid parameters");
		return -EINVAL;
	}

	if (lcz_sock_valid(p)) {
		LOG_ERR("Sock already open");
		return -EPERM;
	}

	int status = -EPERM;
	memcpy(&p->host_addr, addr, sizeof(struct sockaddr));
	if (p->use_dtls) {
		if (p->load_credentials != NULL) {
			status = p->load_credentials();
			if (status < 0) {
				return status;
			}
		}
	}

	if (p->use_dtls) {
		p_sock = socket(p->host_addr.sa_family, SOCK_DGRAM,
				IPPROTO_DTLS_1_2);
	} else {
		p_sock =
			socket(p->host_addr.sa_family, SOCK_DGRAM, IPPROTO_UDP);
	}

	if (p_sock < 0) {
		LOG_ERR("Failed to create socket: -%d", errno);
		return -errno;
	}

	if (p->use_dtls) {
		status = setsockopt(p_sock, SOL_TLS, TLS_SEC_TAG_LIST,
				    p->tls_tag_list, p->list_size);
		if (status < 0) {
			LOG_ERR("Failed to set TLS_SEC_TAG_LIST option: -%d",
				errno);
			return -errno;
		}

		if (name != NULL) {
			status = setsockopt(p_sock, SOL_TLS, TLS_HOSTNAME, name,
					    strlen(name) + 1);
		} else {
			status = setsockopt(p_sock, SOL_TLS, TLS_HOSTNAME, NULL,
					    1);
		}

		if (status < 0) {
			LOG_ERR("Unable to set socket host name '%s': -%d",
				name ? log_strdup(name) : "null", errno);
			return -errno;
		} else {
			LOG_DBG("Set DTLS socket host name: %s",
				name ? log_strdup(name) : "null (disabled)");
		}
	}

	if (connect(p_sock, &p->host_addr, NET_SOCKADDR_MAX_SIZE) < 0) {
		LOG_ERR("Cannot connect UDP: -%d", errno);
		return -errno;
	} else {
		lcz_sock_prepare_fds(p);
	}

	return 0;
}

int lcz_sock_close(sock_info_t *p)
{
	LOG_DBG("Closing socket");
	if (p == NULL) {
		LOG_DBG("Invalid parameters");
		return -EINVAL;
	}

	lcz_sock_clear_fds(p);
	return close(p_sock);
}

int lcz_sock_send(sock_info_t *p, void *data, size_t length, int flags)
{
	if (p == NULL || data == NULL) {
		LOG_DBG("Invalid parameters");
		return -EINVAL;
	}

	int r = send(p_sock, data, length, flags);
	if (r < 0) {
		LOG_ERR("Unable to send: %d", r);
	}
	return r;
}

int lcz_sock_receive(sock_info_t *p, void *data, size_t max_size,
		     int timeout_ms)
{
	if (p == NULL || data == NULL) {
		LOG_DBG("Invalid parameters");
		return -EINVAL;
	}

	memset(data, 0, max_size);

#ifdef CONFIG_LCZ_SOCK_TIMING
	uint32_t start_time = k_cycle_get_32();
#endif

	(void)lcz_sock_wait(p, timeout_ms);

	int count = recv(p_sock, data, max_size, MSG_DONTWAIT);
	if (count == 0) {
		LOG_ERR("No data received");
		count = -ENODATA;
	} else if (count < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			LOG_ERR("Would Block");
			count = -errno;
		} else {
			LOG_ERR("Receive Error");
			count = -errno;
		}
	} else {
		LOG_DBG("%u bytes rxed", count);
	}
#ifdef CONFIG_LCZ_SOCK_TIMING
	uint32_t stop_time = k_cycle_get_32();
	LOG_INF("wait ticks: %u", stop_time - start_time);
#endif
	return count;
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
/* set the number of sockets to 1 */
static void lcz_sock_prepare_fds(sock_info_t *p)
{
	if (p != NULL) {
		p->nfds = 1;
	}
}

static void lcz_sock_clear_fds(sock_info_t *p)
{
	if (p != NULL) {
		p->nfds = 0;
	}
}