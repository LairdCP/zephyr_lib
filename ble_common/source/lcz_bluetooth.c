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
#include <bluetooth/conn.h>

#include "laird_utility_macros.h"
#include "lcz_bluetooth.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define BT_ATT_ERR_SUCCESS 0x00

#define PSCRS PREFIXED_SWITCH_CASE_RETURN_STRING

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
			} else if (gatt[i].uuid->type == BT_UUID_TYPE_128) {
				if (memcmp(gatt[i].uuid, uuid,
					   sizeof(struct bt_uuid_128)) == 0) {
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
	switch (code) {
		PSCRS(BT_ATT_ERR, SUCCESS);
		PSCRS(BT_ATT_ERR, INVALID_HANDLE);
		PSCRS(BT_ATT_ERR, READ_NOT_PERMITTED);
		PSCRS(BT_ATT_ERR, WRITE_NOT_PERMITTED);
		PSCRS(BT_ATT_ERR, INVALID_PDU);
		PSCRS(BT_ATT_ERR, AUTHENTICATION);
		PSCRS(BT_ATT_ERR, NOT_SUPPORTED);
		PSCRS(BT_ATT_ERR, INVALID_OFFSET);
		PSCRS(BT_ATT_ERR, AUTHORIZATION);
		PSCRS(BT_ATT_ERR, PREPARE_QUEUE_FULL);
		PSCRS(BT_ATT_ERR, ATTRIBUTE_NOT_FOUND);
		PSCRS(BT_ATT_ERR, ATTRIBUTE_NOT_LONG);
		PSCRS(BT_ATT_ERR, ENCRYPTION_KEY_SIZE);
		PSCRS(BT_ATT_ERR, INVALID_ATTRIBUTE_LEN);
		PSCRS(BT_ATT_ERR, UNLIKELY);
		PSCRS(BT_ATT_ERR, INSUFFICIENT_ENCRYPTION);
		PSCRS(BT_ATT_ERR, UNSUPPORTED_GROUP_TYPE);
		PSCRS(BT_ATT_ERR, INSUFFICIENT_RESOURCES);
		PSCRS(BT_ATT_ERR, DB_OUT_OF_SYNC);
		PSCRS(BT_ATT_ERR, VALUE_NOT_ALLOWED);
		PSCRS(BT_ATT_ERR, WRITE_REQ_REJECTED);
		PSCRS(BT_ATT_ERR, CCC_IMPROPER_CONF);
		PSCRS(BT_ATT_ERR, PROCEDURE_IN_PROGRESS);
		PSCRS(BT_ATT_ERR, OUT_OF_RANGE);
	default:
		return "UNKNOWN";
	}
}

const char *lbt_get_hci_err_string(uint8_t code)
{
	switch (code) {
		PSCRS(BT_HCI_ERR, SUCCESS);
		PSCRS(BT_HCI_ERR, UNKNOWN_CMD);
		PSCRS(BT_HCI_ERR, UNKNOWN_CONN_ID);
		PSCRS(BT_HCI_ERR, HW_FAILURE);
		PSCRS(BT_HCI_ERR, PAGE_TIMEOUT);
		PSCRS(BT_HCI_ERR, AUTH_FAIL);
		PSCRS(BT_HCI_ERR, PIN_OR_KEY_MISSING);
		PSCRS(BT_HCI_ERR, MEM_CAPACITY_EXCEEDED);
		PSCRS(BT_HCI_ERR, CONN_TIMEOUT);
		PSCRS(BT_HCI_ERR, CONN_LIMIT_EXCEEDED);
		PSCRS(BT_HCI_ERR, SYNC_CONN_LIMIT_EXCEEDED);
		PSCRS(BT_HCI_ERR, CONN_ALREADY_EXISTS);
		PSCRS(BT_HCI_ERR, CMD_DISALLOWED);
		PSCRS(BT_HCI_ERR, INSUFFICIENT_RESOURCES);
		PSCRS(BT_HCI_ERR, INSUFFICIENT_SECURITY);
		PSCRS(BT_HCI_ERR, BD_ADDR_UNACCEPTABLE);
		PSCRS(BT_HCI_ERR, CONN_ACCEPT_TIMEOUT);
		PSCRS(BT_HCI_ERR, UNSUPP_FEATURE_PARAM_VAL);
		PSCRS(BT_HCI_ERR, INVALID_PARAM);
		PSCRS(BT_HCI_ERR, REMOTE_USER_TERM_CONN);
		PSCRS(BT_HCI_ERR, REMOTE_LOW_RESOURCES);
		PSCRS(BT_HCI_ERR, REMOTE_POWER_OFF);
		PSCRS(BT_HCI_ERR, LOCALHOST_TERM_CONN);
		PSCRS(BT_HCI_ERR, PAIRING_NOT_ALLOWED);
		PSCRS(BT_HCI_ERR, UNSUPP_REMOTE_FEATURE);
		PSCRS(BT_HCI_ERR, INVALID_LL_PARAM);
		PSCRS(BT_HCI_ERR, UNSPECIFIED);
		PSCRS(BT_HCI_ERR, UNSUPP_LL_PARAM_VAL);
		PSCRS(BT_HCI_ERR, LL_RESP_TIMEOUT);
		PSCRS(BT_HCI_ERR, LL_PROC_COLLISION);
		PSCRS(BT_HCI_ERR, INSTANT_PASSED);
		PSCRS(BT_HCI_ERR, PAIRING_NOT_SUPPORTED);
		PSCRS(BT_HCI_ERR, DIFF_TRANS_COLLISION);
		PSCRS(BT_HCI_ERR, UNACCEPT_CONN_PARAM);
		PSCRS(BT_HCI_ERR, ADV_TIMEOUT);
		PSCRS(BT_HCI_ERR, TERM_DUE_TO_MIC_FAIL);
		PSCRS(BT_HCI_ERR, CONN_FAIL_TO_ESTAB);
		PSCRS(BT_HCI_ERR, MAC_CONN_FAILED);
		PSCRS(BT_HCI_ERR, CLOCK_ADJUST_REJECTED);
		PSCRS(BT_HCI_ERR, SUBMAP_NOT_DEFINED);
		PSCRS(BT_HCI_ERR, UNKNOWN_ADV_IDENTIFIER);
		PSCRS(BT_HCI_ERR, LIMIT_REACHED);
		PSCRS(BT_HCI_ERR, OP_CANCELLED_BY_HOST);
		PSCRS(BT_HCI_ERR, PACKET_TOO_LONG);
	default:
		return "UNKNOWN";
	}
}

const char *lbt_get_security_err_string(uint8_t code)
{
	switch (code) {
		PSCRS(BT_SECURITY_ERR, SUCCESS);
		PSCRS(BT_SECURITY_ERR, AUTH_FAIL);
		PSCRS(BT_SECURITY_ERR, PIN_OR_KEY_MISSING);
		PSCRS(BT_SECURITY_ERR, OOB_NOT_AVAILABLE);
		PSCRS(BT_SECURITY_ERR, AUTH_REQUIREMENT);
		PSCRS(BT_SECURITY_ERR, PAIR_NOT_SUPPORTED);
		PSCRS(BT_SECURITY_ERR, PAIR_NOT_ALLOWED);
		PSCRS(BT_SECURITY_ERR, INVALID_PARAM);
		PSCRS(BT_SECURITY_ERR, UNSPECIFIED);
	default:
		return "UNKNOWN";
	}
}

bool lbt_central_role(struct bt_conn *conn)
{
	struct bt_conn_info info;
	int rc = bt_conn_get_info(conn, &info);
	if (rc == 0) {
		return (info.role == BT_CONN_ROLE_CENTRAL);
	} else {
		return false;
	}
}

bool lbt_peripheral_role(struct bt_conn *conn)
{
	struct bt_conn_info info;
	int rc = bt_conn_get_info(conn, &info);
	if (rc == 0) {
		return (info.role == BT_CONN_ROLE_PERIPHERAL);
	} else {
		return false;
	}
}
