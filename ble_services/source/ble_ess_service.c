/**
 * @file ble_ess_service.c
 * @brief Provides a BLE environmental sensing service
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <logging/log.h>
LOG_MODULE_REGISTER(ble_ess_svc, CONFIG_BLE_ESS_SERVICE_LOG_LEVEL);

#define ESS_SVC_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#define ESS_SVC_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
#define ESS_SVC_LOG_INF(...) LOG_INF(__VA_ARGS__)
#define ESS_SVC_LOG_DBG(...) LOG_DBG(__VA_ARGS__)

#define ESS_STRING_TEMPERATURE "Temperature"
#define ESS_STRING_HUMIDITY    "Humidity"
#define ESS_STRING_PRESSURE    "Pressure"
#define ESS_STRING_DEW_POINT   "Dew Point"
#define ESS_STRING_ERROR       "Error"

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr/types.h>
#include <sys/byteorder.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>

#include "lcz_bluetooth.h"
#include "ble_ess_service.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/

struct temperature_sensor_s {
	int16_t temperature_value;
};

struct humidity_sensor_s {
	int16_t humidity_value;
};

struct pressure_sensor_s {
	int32_t pressure_value;
};

struct dew_point_sensor_s {
	int8_t dew_point_value;
};

struct ble_ess_service {
	struct temperature_sensor_s temperature_sensor;
	struct humidity_sensor_s humidity_sensor;
	struct pressure_sensor_s pressure_sensor;
	struct dew_point_sensor_s dew_point_sensor;

	uint16_t temperature_index;
	uint16_t humidity_index;
	uint16_t pressure_index;
	uint16_t dew_point_index;
};

struct ccc_table {
	struct lbt_ccc_element temperature;
	struct lbt_ccc_element humidity;
	struct lbt_ccc_element pressure;
	struct lbt_ccc_element dew_point;
};

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static struct ble_ess_service ess;
static struct ccc_table ccc;
static struct bt_conn *ess_svc_conn = NULL;

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static void temperature_ccc_handler(const struct bt_gatt_attr *attr,
				    uint16_t value);

static void humidity_ccc_handler(const struct bt_gatt_attr *attr,
				 uint16_t value);

static void pressure_ccc_handler(const struct bt_gatt_attr *attr,
				 uint16_t value);

static void dew_point_ccc_handler(const struct bt_gatt_attr *attr,
				  uint16_t value);

static void ess_svc_connected(struct bt_conn *conn, uint8_t err);

static void ess_svc_disconnected(struct bt_conn *conn, uint8_t reason);

/******************************************************************************/
/* ESS Service Declaration                                                    */
/******************************************************************************/
static struct bt_gatt_attr ess_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_ESS),

	/* Temperature Characteristic */
	BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       lbt_read_u16, NULL, &ess.temperature_sensor.temperature_value),
	LBT_GATT_CCC(temperature),

	/* Humidity Characteristic */
	BT_GATT_CHARACTERISTIC(BT_UUID_HUMIDITY,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       lbt_read_u16, NULL, &ess.humidity_sensor.humidity_value),
	LBT_GATT_CCC(humidity),

	/* Pressure Characteristic */
	BT_GATT_CHARACTERISTIC(BT_UUID_PRESSURE,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       lbt_read_u32, NULL, &ess.pressure_sensor.pressure_value),
	LBT_GATT_CCC(pressure),

	/* Dew-Point Characteristic */
	BT_GATT_CHARACTERISTIC(BT_UUID_DEW_POINT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       lbt_read_u8, NULL, &ess.dew_point_sensor.dew_point_value),
	LBT_GATT_CCC(dew_point),
};

static struct bt_gatt_service ess_svc = BT_GATT_SERVICE(ess_attrs);

static struct bt_conn_cb ess_svc_conn_callbacks = {
	.connected = ess_svc_connected,
	.disconnected = ess_svc_disconnected,
};

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/

/* Temperature format is in degrees celsius in 0.01 units (e.g. value of 391 is 3.91c) */
void ess_svc_update_temperature(struct bt_conn *conn, int16_t value)
{
	/* Update temperature */
	ess.temperature_sensor.temperature_value = value;

	if (ccc.temperature.notify) {
		/* Send notification */
		value = sys_cpu_to_le16(value);
		bt_gatt_notify(conn, &ess_svc.attrs[ess.temperature_index], &value, sizeof(value));
	}
}

/* Humidity format is in percent in 0.01 units (e.g. value of 524 is 5.24%) */
void ess_svc_update_humidity(struct bt_conn *conn, int16_t value)
{
	/* Update humidity */
	ess.humidity_sensor.humidity_value = value;

	if (ccc.humidity.notify) {
		/* Send notification */
		value = sys_cpu_to_le16(value);
		bt_gatt_notify(conn, &ess_svc.attrs[ess.humidity_index], &value, sizeof(value));
	}
}

/* Pressure format is in pascals in 0.1 units (e.g. value of 86 is 8.6Pa) */
void ess_svc_update_pressure(struct bt_conn *conn, int32_t value)
{
	/* Update pressure */
	ess.pressure_sensor.pressure_value = value;

	if (ccc.pressure.notify) {
		/* Send notification */
		value = sys_cpu_to_le32(value);
		bt_gatt_notify(conn, &ess_svc.attrs[ess.pressure_index], &value, sizeof(value));
	}
}

/* Dew point format is in degress celcius in 1 units (e.g. value of 5 is 5c) */
void ess_svc_update_dew_point(struct bt_conn *conn, int8_t value)
{
	/* Update dew point */
	ess.dew_point_sensor.dew_point_value = value;

	if (ccc.dew_point.notify) {
		/* Send notification */
		bt_gatt_notify(conn, &ess_svc.attrs[ess.dew_point_index], &value, sizeof(value));
	}
}

void ess_svc_init()
{
	bt_gatt_service_register(&ess_svc);

	bt_conn_cb_register(&ess_svc_conn_callbacks);

	/* Calculate indexes of characteristics */
	size_t gatt_size = (sizeof(ess_attrs) / sizeof(ess_attrs[0]));
	ess.temperature_index = lbt_find_gatt_index(BT_UUID_TEMPERATURE, ess_attrs, gatt_size);
	ess.humidity_index = lbt_find_gatt_index(BT_UUID_HUMIDITY, ess_attrs, gatt_size);
	ess.pressure_index = lbt_find_gatt_index(BT_UUID_PRESSURE, ess_attrs, gatt_size);
	ess.dew_point_index = lbt_find_gatt_index(BT_UUID_DEW_POINT, ess_attrs, gatt_size);

	/* Reset all readings to defaults */
	ess.temperature_sensor.temperature_value = 0;
	ess.humidity_sensor.humidity_value = 0;
	ess.pressure_sensor.pressure_value = 0;
	ess.dew_point_sensor.dew_point_value = 0;
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static void temperature_ccc_handler(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	ccc.temperature.notify = IS_NOTIFIABLE(value);
	ess_notifications_changed(ESS_TYPE_TEMPERATURE, ccc.temperature.notify);
}

static void humidity_ccc_handler(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	ccc.humidity.notify = IS_NOTIFIABLE(value);
	ess_notifications_changed(ESS_TYPE_HUMIDITY, ccc.humidity.notify);
}

static void pressure_ccc_handler(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	ccc.pressure.notify = IS_NOTIFIABLE(value);
	ess_notifications_changed(ESS_TYPE_PRESSURE, ccc.pressure.notify);
}

static void dew_point_ccc_handler(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	ccc.dew_point.notify = IS_NOTIFIABLE(value);
	ess_notifications_changed(ESS_TYPE_DEW_POINT, ccc.dew_point.notify);
}

static void ess_svc_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		return;
	}

	if (!lbt_peripheral_role(conn)) {
		return;
	}

	ess_svc_conn = bt_conn_ref(conn);
}

static void ess_svc_disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (!lbt_peripheral_role(conn)) {
		return;
	}

	if (ess_svc_conn) {
		bt_conn_unref(ess_svc_conn);
		ess_svc_conn = NULL;
	}
}

/* The weak implementation can be used for single peripheral designs. */
__weak struct bt_conn *ess_svc_get_conn(void)
{
	return ess_svc_conn;
}

/* Converts an ESS type to a string */
char *ess_type_to_string(enum ess_types_t sensor)
{
	if (sensor == ESS_TYPE_TEMPERATURE) {
		return ESS_STRING_TEMPERATURE;
	}
	else if (sensor == ESS_TYPE_HUMIDITY) {
		return ESS_STRING_HUMIDITY;
	}
	else if (sensor == ESS_TYPE_PRESSURE) {
		return ESS_STRING_PRESSURE;
	}
	else if (sensor == ESS_TYPE_DEW_POINT) {
		return ESS_STRING_DEW_POINT;
	}

	return ESS_STRING_ERROR;
}

/* This weak implementation can be overridden by applications that need it */
__weak void ess_notifications_changed(enum ess_types_t sensor, bool enabled)
{
	ESS_SVC_LOG_DBG("Notifications of %s %sabled", ess_type_to_string(sensor), (enabled == true ? "en" : "dis"));
	return;
}
