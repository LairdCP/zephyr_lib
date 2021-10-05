/**
 * @file lcz_bt_scan.c
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_bt_scan, CONFIG_LCZ_BT_SCAN_LOG_LEVEL);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <kernel.h>
#include <stddef.h>

#include "lcz_bt_scan.h"

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static int scan_start(void);
static bool valid_user_id(int id);
static void lcz_bt_scan_adv_handler(const bt_addr_le_t *addr, int8_t rssi,
				    uint8_t type, struct net_buf_simple *ad);

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static struct {
	atomic_t scanning;
	atomic_t users;
	atomic_t stop_requests;
	atomic_t start_requests;
	bt_le_scan_cb_t *adv_handlers[CONFIG_LCZ_BT_SCAN_MAX_USERS];

	uint32_t num_stops;
	uint32_t num_starts;
} bts;

static struct bt_le_scan_param scan_parameters = BT_LE_SCAN_PARAM_INIT(
	BT_LE_SCAN_TYPE_PASSIVE, BT_LE_SCAN_OPT_FILTER_DUPLICATE,
	CONFIG_LCZ_BT_SCAN_DEFAULT_INTERVAL, CONFIG_LCZ_BT_SCAN_DEFAULT_WINDOW);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
bool lcz_bt_scan_set_parameters(const struct bt_le_scan_param *param)
{
	if (atomic_get(&bts.users) != 0) {
		return false;
	} else {
		memcpy(&scan_parameters, param, sizeof(scan_parameters));
		return true;
	}
}

bool lcz_bt_scan_register(int *pId, bt_le_scan_cb_t *cb)
{
	*pId = (int)atomic_inc(&bts.users);

	if (valid_user_id(*pId)) {
		bts.adv_handlers[*pId] = cb;
		return true;
	} else {
		return false;
	}
}

int lcz_bt_scan_start(int id)
{
	int r = -EPERM;

	if (valid_user_id(id)) {
		atomic_set_bit(&bts.start_requests, id);
		r = scan_start();
	}

	return r;
}

int lcz_bt_scan_stop(int id)
{
	int r = -EPERM;

	if (valid_user_id(id)) {
		atomic_clear_bit(&bts.start_requests, id);
		atomic_set_bit(&bts.stop_requests, id);
		if (atomic_cas(&bts.scanning, 1, 0)) {
			r = bt_le_scan_stop();
			LOG_DBG("%d", r);
			if (r == 0) {
				bts.num_stops += 1;
			}
		} else {
			r = 0;
		}
	}

	return r;
}

int lcz_bt_scan_resume(int id)
{
	int r = -EPERM;

	if (valid_user_id(id)) {
		atomic_clear_bit(&bts.stop_requests, id);
		r = scan_start();
	}

	return r;
}

int lcz_bt_scan_restart(int id)
{
	int r = -EPERM;

	if (valid_user_id(id)) {
		atomic_clear_bit(&bts.stop_requests, id);
		atomic_set_bit(&bts.start_requests, id);
		r = scan_start();
	}

	return r;
}

bool lcz_bt_scan_active(void)
{
	return (atomic_get(&bts.scanning) != 0);
}

uint32_t lcz_bt_scan_get_num_starts(void)
{
	return bts.num_starts;
}

uint32_t lcz_bt_scan_get_num_stops(void)
{
	return bts.num_stops;
}

int lcz_bt_scan_update_parameters(int id, const struct bt_le_scan_param *param)
{
	int r = -EPERM;

	if (valid_user_id(id)) {
		r = lcz_bt_scan_stop(id);
		if (r == 0) {
			memcpy(&scan_parameters, param,
			       sizeof(scan_parameters));
			r = lcz_bt_scan_restart(id);
		}
	}

	return r;
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static int scan_start(void)
{
	int r = 0;

	/* Return success if scanning can't be started now.
	 * This is a multiuser system.
	 */
	if (atomic_get(&bts.start_requests) == 0) {
		return r;
	}

	if (atomic_get(&bts.stop_requests) != 0) {
		return r;
	}

	if (atomic_cas(&bts.scanning, 0, 1)) {
		r = bt_le_scan_start(&scan_parameters, lcz_bt_scan_adv_handler);
		if (r != 0) {
			LOG_ERR("Unable to start scanning: %d", r);
			atomic_clear(&bts.scanning);
		} else {
			LOG_DBG("%d", r);
			bts.num_starts += 1;
		}
	}

	return r;
}

static bool valid_user_id(int id)
{
	if (id >= 0 && id < CONFIG_LCZ_BT_SCAN_MAX_USERS) {
		return true;
	} else {
		LOG_ERR("Invalid bt scan user id");
		return false;
	}
}

static void lcz_bt_scan_adv_handler(const bt_addr_le_t *addr, int8_t rssi,
				    uint8_t type, struct net_buf_simple *ad)
{
#ifdef CONFIG_LCZ_BT_SCAN_VERBOSE_ADV_HANDLER
	char bt_addr[BT_ADDR_LE_STR_LEN];
	memset(bt_addr, 0, BT_ADDR_LE_STR_LEN);
	bt_addr_le_to_str(addr, bt_addr, BT_ADDR_LE_STR_LEN);
	LOG_DBG("Advert from %s RSSI: %d Type: %d", log_strdup(bt_addr), rssi,
		type);
	LOG_HEXDUMP_DBG(ad->data, ad->len, "Data:");
#endif

	size_t i;
	for (i = 0; i < CONFIG_LCZ_BT_SCAN_MAX_USERS; i++) {
		if (bts.adv_handlers[i] != NULL) {
			bts.adv_handlers[i](addr, rssi, type, ad);
		}
	}
}
