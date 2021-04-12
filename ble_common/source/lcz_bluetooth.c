/**
 * @file lcz_bluetooth.c
 * @brief Common Bluetooth operations.
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(lcz_bluetooth);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <bluetooth/gatt.h>
#include <nrfx.h>
#include <hal/nrf_power.h>
#if !NRF_POWER_HAS_RESETREAS
#include <hal/nrf_reset.h>
#endif

#include "laird_utility_macros.h"
#include "lcz_bluetooth.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define BT_ATT_ERR_SUCCESS 0x00

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
ssize_t lbt_read_u8(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		    void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 sizeof(uint8_t));
}

ssize_t lbt_read_u16(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		     void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 sizeof(uint16_t));
}

ssize_t lbt_read_u32(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		     void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 sizeof(uint32_t));
}

ssize_t lbt_read_integer(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 sizeof(int));
}

ssize_t lbt_read_string(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset,
			uint16_t max_str_length)
{
	const char *value = attr->user_data;
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 MIN(strlen(value), max_str_length));
}

ssize_t lbt_read_string_no_max_size(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr, void *buf,
				    uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

ssize_t lbt_write_string(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags, uint16_t max_str_length)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(flags);

	if ((offset + len) > max_str_length) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	char *value = attr->user_data;
	memcpy(value + offset, buf, len);
	/* null terminate the value that was written */
	*(value + offset + len) = 0;

	return len;
}

ssize_t lbt_write_u8(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		     const void *buf, uint16_t len, uint16_t offset,
		     uint8_t flags)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(flags);

	if (offset != 0 || len != sizeof(uint8_t)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(attr->user_data, buf, len);
	return len;
}

ssize_t lbt_write_u16(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		      const void *buf, uint16_t len, uint16_t offset,
		      uint8_t flags)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(flags);

	if (offset != 0 || len != sizeof(uint16_t)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(attr->user_data, buf, len);
	return len;
}

uint16_t lbt_find_gatt_index(struct bt_uuid *uuid, struct bt_gatt_attr *gatt,
			     size_t size)
{
	size_t i = 0;
	while (i < size) {
		if (gatt[i].uuid->type == uuid->type) {
			if (gatt[i].uuid->type == BT_UUID_TYPE_16) {
				if (((struct bt_uuid_16 *)uuid)->val ==
				    ((struct bt_uuid_16 *)gatt[i].uuid)->val) {
					return (uint16_t)i;
				}
			}
			else if (gatt[i].uuid->type == BT_UUID_TYPE_128) {
				if (memcmp(gatt[i].uuid, uuid, sizeof(struct bt_uuid_128)) ==
				    0) {
					return (uint16_t)i;
				}
			}
		}
		++i;
	}

	/* Not found */
	__ASSERT(0, "GATT handle for characteristic not found");
	return 0;
}

const char *lbt_get_att_err_string(uint8_t code)
{
	/* clang-format off */
	switch (code) {
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, SUCCESS);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, INVALID_HANDLE);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, READ_NOT_PERMITTED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, WRITE_NOT_PERMITTED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, INVALID_PDU);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, AUTHENTICATION);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, NOT_SUPPORTED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, INVALID_OFFSET);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, AUTHORIZATION);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, PREPARE_QUEUE_FULL);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, ATTRIBUTE_NOT_FOUND);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, ATTRIBUTE_NOT_LONG);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, ENCRYPTION_KEY_SIZE);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, INVALID_ATTRIBUTE_LEN);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, UNLIKELY);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, INSUFFICIENT_ENCRYPTION);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, UNSUPPORTED_GROUP_TYPE);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, INSUFFICIENT_RESOURCES);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, DB_OUT_OF_SYNC);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, VALUE_NOT_ALLOWED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, WRITE_REQ_REJECTED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, CCC_IMPROPER_CONF);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, PROCEDURE_IN_PROGRESS);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_ATT_ERR, OUT_OF_RANGE);
	default:
		return "UNKNOWN";
	}
	/* clang-format on */
}

const char *lbt_get_hci_err_string(uint8_t code)
{
	/* clang-format off */
	switch (code) {
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, SUCCESS);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, UNKNOWN_CMD);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, UNKNOWN_CONN_ID);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, HW_FAILURE);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, PAGE_TIMEOUT);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, AUTH_FAIL);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, PIN_OR_KEY_MISSING);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, MEM_CAPACITY_EXCEEDED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, CONN_TIMEOUT);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, CONN_LIMIT_EXCEEDED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, SYNC_CONN_LIMIT_EXCEEDED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, CONN_ALREADY_EXISTS);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, CMD_DISALLOWED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, INSUFFICIENT_RESOURCES);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, INSUFFICIENT_SECURITY);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, BD_ADDR_UNACCEPTABLE);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, CONN_ACCEPT_TIMEOUT);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, UNSUPP_FEATURE_PARAM_VAL);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, INVALID_PARAM);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, REMOTE_USER_TERM_CONN);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, REMOTE_LOW_RESOURCES);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, REMOTE_POWER_OFF);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, LOCALHOST_TERM_CONN);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, PAIRING_NOT_ALLOWED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, UNSUPP_REMOTE_FEATURE);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, INVALID_LL_PARAM);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, UNSPECIFIED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, UNSUPP_LL_PARAM_VAL);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, LL_RESP_TIMEOUT);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, LL_PROC_COLLISION);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, INSTANT_PASSED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, PAIRING_NOT_SUPPORTED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, DIFF_TRANS_COLLISION);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, UNACCEPT_CONN_PARAM);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, ADV_TIMEOUT);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, TERM_DUE_TO_MIC_FAIL);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, CONN_FAIL_TO_ESTAB);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, MAC_CONN_FAILED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, CLOCK_ADJUST_REJECTED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, SUBMAP_NOT_DEFINED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, UNKNOWN_ADV_IDENTIFIER);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, LIMIT_REACHED);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, OP_CANCELLED_BY_HOST);
		PREFIXED_SWITCH_CASE_RETURN_STRING(BT_HCI_ERR, PACKET_TOO_LONG);
	default:
		return "UNKNOWN";
	}
	/* clang-format on */
}

uint32_t lbt_get_and_clear_nrf52_reset_reason_register(void)
{
	uint32_t reason;

#if NRF_POWER_HAS_RESETREAS
	reason = nrf_power_resetreas_get(NRF_POWER);
#else
	reason = nrf_reset_resetreas_get(NRF_RESET);
#endif

#if NRF_POWER_HAS_RESETREAS
	nrf_power_resetreas_clear(NRF_POWER, reason);
#else
	nrf_reset_resetreas_clear(NRF_RESET, reason);
#endif

	return reason;
}

const char *lbt_get_nrf52_reset_reason_string_from_register(uint32_t reg)
{
	if (reg == 0) {
		return "POWER_UP";
	}

	/* Priority affects the result because multiple bits can be set. */
#if !NRF_POWER_HAS_RESETREAS
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, RESETPIN, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, DOG0, Msk);
	IF_MASK_SET_RETURN_STRING(reg, NRFX_RESET_REASON, DOG0, MASK);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, CTRLAP, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, SREQ, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, LOCKUP, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, OFF, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, LPCOMP, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, DIF, Msk);
#if NRF_RESET_HAS_NETWORK
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, LSREQ, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, LLOCKUP, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, LDOG, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, MFORCEOFF, Msk);
#endif
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, NFC, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, DOG1, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, VBUS, Msk);
#if NRF_RESET_HAS_NETWORK
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, LCTRLAP, Msk);
#endif
#else
	IF_MASK_SET_RETURN_STRING(reg, POWER_RESETREAS, RESETPIN, Msk);
	IF_MASK_SET_RETURN_STRING(reg, POWER_RESETREAS, DOG, Msk);
	IF_MASK_SET_RETURN_STRING(reg, POWER_RESETREAS, SREQ, Msk);
	IF_MASK_SET_RETURN_STRING(reg, POWER_RESETREAS, LOCKUP, Msk);
	IF_MASK_SET_RETURN_STRING(reg, POWER_RESETREAS, OFF, Msk);
#if defined(POWER_RESETREAS_LPCOMP_Msk)
	IF_MASK_SET_RETURN_STRING(reg, POWER_RESETREAS, LPCOMP, Msk);
#endif
	IF_MASK_SET_RETURN_STRING(reg, POWER_RESETREAS, DIF, Msk);
#if defined(POWER_RESETREAS_NFC_Msk)
	IF_MASK_SET_RETURN_STRING(reg, POWER_RESETREAS, NFC, Msk);
#endif
#if defined(POWER_RESETREAS_VBUS_Msk)
	IF_MASK_SET_RETURN_STRING(reg, POWER_RESETREAS, VBUS, Msk);
#endif
#endif
	return "UNKNOWN";
}

bool lbt_central_role(struct bt_conn *conn)
{
	struct bt_conn_info info;
	int rc = bt_conn_get_info(conn, &info);
	if (rc == 0) {
		return (info.role == BT_CONN_ROLE_MASTER);
	} else {
		return false;
	}
}

bool lbt_peripheral_role(struct bt_conn *conn)
{
	struct bt_conn_info info;
	int rc = bt_conn_get_info(conn, &info);
	if (rc == 0) {
		return (info.role == BT_CONN_ROLE_SLAVE);
	} else {
		return false;
	}
}
