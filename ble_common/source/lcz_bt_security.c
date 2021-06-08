/**
 * @file lcz_bt_security.c
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_bt_security, CONFIG_LCZ_BT_SECURITY_LOG_LEVEL);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <init.h>
#include <spinlock.h>

#include "lcz_bluetooth.h"
#include "lcz_bt_security.h"

#ifdef CONFIG_BT_SMP_APP_PAIRING_ACCEPT
#warning "not supported"
#endif

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#if defined(CONFIG_LCZ_BT_SECURITY_HAS_BUTTON) &&                              \
	defined(CONFIG_LCZ_BT_SECURITY_HAS_DISPLAY)
#define ENABLE_PASSKEY_CONFIRM 1
#endif

#define LCZ_SEC_DISPATCHER_1(func, p1)                                         \
	do {                                                                   \
		LOG_DBG(".");                                                  \
                                                                               \
		size_t i;                                                      \
		for (i = 0; i < lcz_sec_users; i++) {                          \
			if (lcz_sec_callbacks[i]->func != NULL) {              \
				lcz_sec_callbacks[i]->func(p1);                \
			}                                                      \
		}                                                              \
	} while (0);

#define LCZ_SEC_DISPATCHER_2(func, p1, p2)                                     \
	do {                                                                   \
		LOG_DBG(".");                                                  \
                                                                               \
		size_t i;                                                      \
		for (i = 0; i < lcz_sec_users; i++) {                          \
			if (lcz_sec_callbacks[i]->func != NULL) {              \
				lcz_sec_callbacks[i]->func(p1, p2);            \
			}                                                      \
		}                                                              \
	} while (0);

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static struct k_spinlock lcz_sec_lock;
static size_t lcz_sec_users;
static const struct bt_conn_auth_cb
	*lcz_sec_callbacks[CONFIG_LCZ_BT_SECURITY_MAX_USERS];

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static int lcz_bt_security_register(const struct device *device);

#ifdef CONFIG_LCZ_BT_SECURITY_HAS_DISPLAY
static void lcz_passkey_display(struct bt_conn *conn, unsigned int passkey);
#endif

#ifdef ENABLE_PASSKEY_CONFIRM
static void lcz_passkey_confirm(struct bt_conn *conn, unsigned int passkey);
#endif

#ifdef CONFIG_LCZ_BT_SECURITY_HAS_KEYBOARD
static void lcz_passkey_entry(struct bt_conn *conn);
#endif

static void lcz_cancel(struct bt_conn *conn);
static void lcz_pairing_confirm(struct bt_conn *conn);
static void lcz_pairing_complete(struct bt_conn *conn, bool bonded);
static void lcz_pairing_failed(struct bt_conn *conn,
			       enum bt_security_err reason);
static void lcz_bond_deleted(uint8_t id, const bt_addr_le_t *peer);

#ifdef CONFIG_LCZ_BT_SECURITY_OOB_PAIRING
static void lcz_oob_data_request(struct bt_conn *conn,
				 struct bt_conn_oob_info *info);
#endif

static void callback_checker(const struct bt_conn_auth_cb *cb);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
SYS_INIT(lcz_bt_security_register, APPLICATION,
	 CONFIG_LCZ_BT_SECURITY_INIT_PRIORITY);

int lcz_bt_security_register_cb(const struct bt_conn_auth_cb *cb)
{
	k_spinlock_key_t key;

	if (lcz_sec_users < CONFIG_LCZ_BT_SECURITY_MAX_USERS) {
		key = k_spin_lock(&lcz_sec_lock);
		lcz_sec_callbacks[lcz_sec_users] = cb;
		lcz_sec_users += 1;
		callback_checker(cb);
		k_spin_unlock(&lcz_sec_lock, key);
		return 0;
	} else {
		LOG_ERR("Unable to register callbacks.  Not enough users");
		return -EPERM;
	}
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static int lcz_bt_security_register(const struct device *device)
{
	ARG_UNUSED(device);
	int r = -EPERM;

	static const struct bt_conn_auth_cb LCZ_BT_AUTH_CALLBACKS = {

#ifdef CONFIG_LCZ_BT_SECURITY_HAS_DISPLAY
		.passkey_display = lcz_passkey_display,
#endif

#ifdef ENABLE_PASSKEY_CONFIRM
		.passkey_confirm = lcz_passkey_confirm,
#endif

#ifdef CONFIG_LCZ_BT_SECURITY_HAS_KEYBOARD
		.passkey_entry = lcz_passkey_entry,
#endif

#ifdef CONFIG_LCZ_BT_SECURITY_OOB_PAIRING
		.oob_data_request = lcz_oob_data_request,
#endif
		/* Wrapper doesn't enforce non-null cancel callback (when required) */
		.cancel = lcz_cancel,

		/* Pairing confirm only is used when none of the above modes are true */
		.pairing_confirm = lcz_pairing_confirm,

		.pairing_complete = lcz_pairing_complete,
		.pairing_failed = lcz_pairing_failed,
		.bond_deleted = lcz_bond_deleted,
	};

	r = bt_conn_auth_cb_register(&LCZ_BT_AUTH_CALLBACKS);
	if (r < 0) {
		LOG_ERR("Register security callback status: %d", r);
	}
	return r;
}

#ifdef CONFIG_LCZ_BT_SECURITY_HAS_DISPLAY
static void lcz_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	LOG_DBG("%d", passkey);
	LCZ_SEC_DISPATCHER_2(passkey_display, conn, passkey);
}
#endif

#ifdef ENABLE_PASSKEY_CONFIRM
static void lcz_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	LCZ_SEC_DISPATCHER_2(passkey_confirm, conn, passkey);
}
#endif

#ifdef CONFIG_LCZ_BT_SECURITY_HAS_KEYBOARD
static void lcz_passkey_entry(struct bt_conn *conn)
{
	LCZ_SEC_DISPATCHER_1(passkey_entry, conn);
}
#endif

static void lcz_cancel(struct bt_conn *conn)
{
	LCZ_SEC_DISPATCHER_1(cancel, conn);
}

static void lcz_pairing_confirm(struct bt_conn *conn)
{
	LCZ_SEC_DISPATCHER_1(pairing_confirm, conn);
}

static void lcz_pairing_complete(struct bt_conn *conn, bool bonded)
{
	LOG_DBG("%s bonded", bonded ? "" : "NOT");
	LCZ_SEC_DISPATCHER_2(pairing_complete, conn, bonded);
}

static void lcz_pairing_failed(struct bt_conn *conn,
			       enum bt_security_err reason)
{
	LOG_ERR("Bluetooth Security error: %d %s", reason,
		lbt_get_security_err_string(reason));
	LCZ_SEC_DISPATCHER_2(pairing_failed, conn, reason);
}

static void lcz_bond_deleted(uint8_t id, const bt_addr_le_t *peer)
{
	LCZ_SEC_DISPATCHER_2(bond_deleted, id, peer);
}

#ifdef CONFIG_LCZ_BT_SECURITY_OOB_PAIRING
static void lcz_oob_data_request(struct bt_conn *conn,
				 struct bt_conn_oob_info *info)
{
	LCZ_SEC_DISPATCHER_2(oob_data_request, conn, info);
}
#endif

static void callback_checker(const struct bt_conn_auth_cb *cb)
{
#ifndef CONFIG_LCZ_BT_SECURITY_HAS_DISPLAY
	if (cb->passkey_display != NULL) {
		LOG_WRN("passkey_display callback not enabled");
	}
#endif

#ifndef ENABLE_PASSKEY_CONFIRM
	if (cb->passkey_confirm != NULL) {
		LOG_WRN("passkey_confirm callback not enabled");
	}
#endif

#ifndef CONFIG_LCZ_BT_SECURITY_HAS_KEYBOARD
	if (cb->passkey_entry != NULL) {
		LOG_WRN("passkey_entry callback not enabled");
	}
#endif

#ifndef CONFIG_LCZ_BT_SECURITY_OOB_PAIRING
	if (cb->oob_data_request != NULL) {
		LOG_WRN("oob_data_request callback not enabled");
	}
#endif

#if defined(CONFIG_LCZ_BT_SECURITY_HAS_DISPLAY) ||                             \
	defined(ENABLE_PASSKEY_CONFIRM) ||                                     \
	defined(CONFIG_LCZ_BT_SECURITY_HAS_KEYBOARD)
	if (cb->cancel == NULL) {
		LOG_WRN("Cancel callback should be provided with interactive modes");
	}
#endif
}