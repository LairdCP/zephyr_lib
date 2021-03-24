/**
 * @file dis.c
 * @brief Zephyr's DIS does not contain all the desired characteristics.
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(dis);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/bluetooth.h>
#include <kernel_version.h>
#include <version.h>

#include "lcz_bluetooth.h"
#include "dis.h"

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/

/* '%c' didn't work properly in string conversion so '%u' was used.
 * Therefore, version string size is larger than Zephyr max of "255.255.255".*/
#define VERSION_MAX_SIZE ((10 * 3) + 2 + 1)
#define VERSION_MAX_STR_LEN (VERSION_MAX_SIZE - 1)

static struct bt_uuid_16 DIS_UUID = BT_UUID_INIT_16(0x180a);
static struct bt_uuid_16 MODEL_NUMBER_UUID = BT_UUID_INIT_16(0x2a24);
static struct bt_uuid_16 FIRMWARE_REVISION_UUID = BT_UUID_INIT_16(0x2a26);
static struct bt_uuid_16 SOFTWARE_REVISION_UUID = BT_UUID_INIT_16(0x2a28);
static struct bt_uuid_16 MANUFACTURER_NAME_UUID = BT_UUID_INIT_16(0x2a29);

static const char MODEL_NUMBER[] = CONFIG_BOARD;
static const char MANUFACTURER_NAME[] = "Laird Connectivity";

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static ssize_t read_const_string(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr, void *buf,
				 uint16_t len, uint16_t offset);

static ssize_t read_version(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset);

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static char firmware_version[VERSION_MAX_SIZE] = KERNEL_VERSION_STRING;
static char software_revision[VERSION_MAX_SIZE];

static struct bt_gatt_attr dis_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(&DIS_UUID),
	BT_GATT_CHARACTERISTIC(&MODEL_NUMBER_UUID.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_const_string, NULL,
			       (char *)MODEL_NUMBER),
	BT_GATT_CHARACTERISTIC(&FIRMWARE_REVISION_UUID.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_version, NULL,
			       firmware_version),
	BT_GATT_CHARACTERISTIC(&SOFTWARE_REVISION_UUID.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_version, NULL,
			       software_revision),
	BT_GATT_CHARACTERISTIC(&MANUFACTURER_NAME_UUID.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_const_string, NULL,
			       (char *)MANUFACTURER_NAME),

};

static struct bt_gatt_service dis_gatt = BT_GATT_SERVICE(dis_attrs);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void dis_initialize(const char *const app_version_string)
{
	strncpy(software_revision, app_version_string, VERSION_MAX_STR_LEN);

	bt_gatt_service_register(&dis_gatt);
}

const char *dis_get_model_number(void)
{
	return MODEL_NUMBER;
}

const char *dis_get_software_revision(void)
{
	return software_revision;
}

const char *dis_get_manufacturer_name(void)
{
	return MANUFACTURER_NAME;
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/

/* Constant strings are assumed to be properly terminated. */
static ssize_t read_const_string(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr, void *buf,
				 uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

/* The Zephyr version is placed in the firmware revision characteristic. */
static ssize_t read_version(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	return lbt_read_string(conn, attr, buf, len, offset,
			       VERSION_MAX_STR_LEN);
}
