/**
 * @file ble_power_service.c
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(ble_power_svc, CONFIG_BLE_POWER_SERVICE_LOG_LEVEL);

#define POWER_SVC_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#define POWER_SVC_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
#define POWER_SVC_LOG_INF(...) LOG_INF(__VA_ARGS__)
#define POWER_SVC_LOG_DBG(...) LOG_DBG(__VA_ARGS__)

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/bluetooth.h>

#include "lcz_bluetooth.h"
#include "ble_power_service.h"
#include "laird_power.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define POWER_SVC_BASE_UUID_128(_x_)                                           \
	BT_UUID_INIT_128(0xeb, 0xb7, 0xb2, 0x67, 0xfb, 0x78, 0x4e, 0xf2, 0x9e, \
			 0x55, 0xd7, 0xf3, LSB_16(_x_), MSB_16(_x_), 0x1c,     \
			 0xdc)
static struct bt_uuid_128 POWER_SVC_UUID = POWER_SVC_BASE_UUID_128(0x0000);
static struct bt_uuid_128 VOLTAGE_UUID = POWER_SVC_BASE_UUID_128(0x0001);
static struct bt_uuid_128 REBOOT_UUID = POWER_SVC_BASE_UUID_128(0x0002);

struct ble_power_voltage {
	uint8_t voltage_int;
	uint8_t voltage_dec;
};

struct ble_power_service {
	struct ble_power_voltage voltage;
#ifdef CONFIG_REBOOT
	uint8_t reboot;
#endif

	uint16_t voltage_index;
};

struct ccc_table {
	struct lbt_ccc_element voltage;
};

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static struct ble_power_service bps;
static struct ccc_table ccc;
static struct bt_conn *power_svc_conn = NULL;

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static void voltage_ccc_handler(const struct bt_gatt_attr *attr,
				uint16_t value);

#ifdef CONFIG_REBOOT
static ssize_t write_power_reboot(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  const void *buf, uint16_t len,
				  uint16_t offset, uint8_t flags);
#endif

static void power_svc_connected(struct bt_conn *conn, uint8_t err);
static void power_svc_disconnected(struct bt_conn *conn, uint8_t reason);

/******************************************************************************/
/* Power Service Declaration                                                  */
/******************************************************************************/
static struct bt_gatt_attr power_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(&POWER_SVC_UUID),
	BT_GATT_CHARACTERISTIC(&VOLTAGE_UUID.uuid, BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, &bps.voltage),
	LBT_GATT_CCC(voltage)
#ifdef CONFIG_REBOOT
		,
	BT_GATT_CHARACTERISTIC(&REBOOT_UUID.uuid, BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE, NULL, write_power_reboot,
			       &bps.reboot),
#endif
};

static struct bt_gatt_service power_svc = BT_GATT_SERVICE(power_attrs);

static struct bt_conn_cb power_svc_conn_callbacks = {
	.connected = power_svc_connected,
	.disconnected = power_svc_disconnected,
};

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
static void power_svc_notify(bool notify, uint16_t index, uint16_t length)
{
	struct bt_conn *connection_handle = power_svc_get_conn();
	if (connection_handle != NULL) {
		if (notify) {
			bt_gatt_notify(connection_handle,
				       &power_svc.attrs[index],
				       power_svc.attrs[index].user_data,
				       length);
		}
	}
}

void power_svc_set_voltage(uint8_t integer, uint8_t decimal)
{
	bps.voltage.voltage_int = integer;
	bps.voltage.voltage_dec = decimal;
	power_svc_notify(ccc.voltage.notify, bps.voltage_index,
			 sizeof(bps.voltage));
}

void power_svc_init()
{
	bt_gatt_service_register(&power_svc);

	bt_conn_cb_register(&power_svc_conn_callbacks);

	size_t gatt_size = (sizeof(power_attrs) / sizeof(power_attrs[0]));
	bps.voltage_index =
		lbt_find_gatt_index(&VOLTAGE_UUID.uuid, power_attrs, gatt_size);
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static void voltage_ccc_handler(const struct bt_gatt_attr *attr, uint16_t value)
{
	ccc.voltage.notify = IS_NOTIFIABLE(value);
	power_mode_set(ccc.voltage.notify);
}

#ifdef CONFIG_REBOOT
static ssize_t write_power_reboot(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  const void *buf, uint16_t len,
				  uint16_t offset, uint8_t flags)
{
	ssize_t length = lbt_write_u8(conn, attr, buf, len, offset, flags);
	if (length > 0) {
		power_reboot_module(bps.reboot);
	}
	return length;
}
#endif

static void power_svc_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		return;
	}

	if (!lbt_peripheral_role(conn)) {
		return;
	}

	power_svc_conn = bt_conn_ref(conn);
}

static void power_svc_disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (!lbt_peripheral_role(conn)) {
		return;
	}

	if (power_svc_conn) {
		bt_conn_unref(power_svc_conn);
		power_svc_conn = NULL;
	}
}

/* The weak implementation can be used for single peripheral designs. */
__weak struct bt_conn *power_svc_get_conn(void)
{
	return power_svc_conn;
}
