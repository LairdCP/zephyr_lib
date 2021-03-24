/**
 * @file fota.c
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(fota);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/bluetooth.h>

#ifdef CONFIG_MODEM_HL7800
#include <drivers/modem/hl7800.h>
#endif

#include "lcz_bluetooth.h"
#include "file_system_utilities.h"
#include "fota.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define FOTA_WORKQ_THREAD_STACK_SIZE 2048
#define FOTA_WORKQ_THREAD_PRIORITY K_PRIO_PREEMPT(2)

#define FOTA_BASE_UUID_128(_x_)                                                \
	BT_UUID_INIT_128(0x45, 0x57, 0x7c, 0x74, 0x49, 0x83, 0x85, 0x26, 0x4b, \
			 0x32, 0x2a, 0x0a, LSB_16(_x_), MSB_16(_x_), 0x12,     \
			 0x3e)

static struct bt_uuid_128 FOTA_UUID = FOTA_BASE_UUID_128(0xf000);
static struct bt_uuid_128 FOTA_CONTROL_POINT_UUID = FOTA_BASE_UUID_128(0xf001);
static struct bt_uuid_128 FOTA_STATUS = FOTA_BASE_UUID_128(0xf002);
static struct bt_uuid_128 FOTA_COUNT = FOTA_BASE_UUID_128(0xf003);
static struct bt_uuid_128 FOTA_SIZE = FOTA_BASE_UUID_128(0xf004);
static struct bt_uuid_128 FOTA_FILE_NAME = FOTA_BASE_UUID_128(0xf005);
static struct bt_uuid_128 FOTA_HASH = FOTA_BASE_UUID_128(0xf006);

struct fota {
	uint8_t control_point;
	int status;
	uint32_t count;
	uint32_t size;
	char file_name[CONFIG_FOTA_FILE_NAME_MAX_SIZE];
	uint8_t hash[FSU_HASH_SIZE];

	uint16_t status_index;
	uint16_t count_index;
	uint16_t size_index;
	uint16_t file_name_index;
	uint16_t hash_index;

	struct k_work_q work_q;
	struct k_work work;
};

struct ccc_table {
	struct lbt_ccc_element status;
	struct lbt_ccc_element count;
	struct lbt_ccc_element size;
	struct lbt_ccc_element file_name;
	struct lbt_ccc_element hash;
};

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static struct fota fota;
static struct ccc_table ccc;
static struct bt_conn *fota_conn = NULL;
static char fota_full_name[MAX_FILE_NAME + 1];

K_THREAD_STACK_DEFINE(fcs_workq_stack, FOTA_WORKQ_THREAD_STACK_SIZE);

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static void fota_connected(struct bt_conn *conn, uint8_t err);
static void fota_disconnected(struct bt_conn *conn, uint8_t reason);
static void fota_notify(bool notify, uint16_t index, uint16_t length);

static ssize_t write_control_point(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len,
				   uint16_t offset, uint8_t flags);

static ssize_t read_file_name(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset);

static ssize_t write_file_name(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, const void *buf,
			       uint16_t len, uint16_t offset, uint8_t flags);

static ssize_t read_hash(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset);

static void status_ccc_handler(const struct bt_gatt_attr *attr, uint16_t value);
static void count_ccc_handler(const struct bt_gatt_attr *attr, uint16_t value);
static void size_ccc_handler(const struct bt_gatt_attr *attr, uint16_t value);
static void file_name_ccc_handler(const struct bt_gatt_attr *attr,
				  uint16_t value);
static void hash_ccc_handler(const struct bt_gatt_attr *attr, uint16_t value);

static void workq_fota_handler(struct k_work *item);

static void fota_list_files(const char *name);
static void fota_modem_start(const char *name);
static void fota_delete_files(const char *name);
static void fota_compute_sha256(const char *name);

static void fota_build_full_name(const char *name);

/******************************************************************************/
/* FOTA Service Declaration                                                   */
/******************************************************************************/
static struct bt_gatt_attr fota_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(&FOTA_UUID),
	BT_GATT_CHARACTERISTIC(&FOTA_CONTROL_POINT_UUID.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       lbt_read_u8, write_control_point,
			       &fota.control_point),
	BT_GATT_CHARACTERISTIC(
		&FOTA_STATUS.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		BT_GATT_PERM_READ, lbt_read_integer, NULL, &fota.status),
	LBT_GATT_CCC(status),
	BT_GATT_CHARACTERISTIC(
		&FOTA_COUNT.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		BT_GATT_PERM_READ, lbt_read_u32, NULL, &fota.count),
	LBT_GATT_CCC(count),
	BT_GATT_CHARACTERISTIC(
		&FOTA_SIZE.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		BT_GATT_PERM_READ, lbt_read_u32, NULL, &fota.size),
	LBT_GATT_CCC(size),
	BT_GATT_CHARACTERISTIC(&FOTA_FILE_NAME.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
				       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_file_name, write_file_name, fota.file_name),
	LBT_GATT_CCC(file_name),
	BT_GATT_CHARACTERISTIC(&FOTA_HASH.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_hash, NULL, &fota.hash),
	LBT_GATT_CCC(hash)
};

static struct bt_gatt_service fota_svc = BT_GATT_SERVICE(fota_attrs);

static struct bt_conn_cb fota_conn_callbacks = {
	.connected = fota_connected,
	.disconnected = fota_disconnected,
};

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void fota_set_status(int status)
{
	fota.status = status;
	fota_notify(ccc.status.notify, fota.status_index, sizeof(fota.status));
}

void fota_set_count(uint32_t count)
{
	fota.count = count;
	fota_notify(ccc.count.notify, fota.count_index, sizeof(fota.count));
}

void fota_set_size(uint32_t size)
{
	fota.size = size;
	fota_notify(ccc.size.notify, fota.size_index, sizeof(fota.size));
}

void fota_set_file_name(const char *name)
{
	__ASSERT_NO_MSG(name != NULL);
	strncpy(fota.file_name, name, CONFIG_FOTA_FILE_NAME_MAX_SIZE - 1);
	fota_notify(ccc.file_name.notify, fota.file_name_index,
		    strlen(fota.file_name));
}

void fota_set_hash(const uint8_t *hash)
{
	__ASSERT_NO_MSG(hash != NULL);
	memcpy(fota.hash, hash, FSU_HASH_SIZE);
	fota_notify(ccc.hash.notify, fota.hash_index, sizeof(fota.hash));
}

void fota_init()
{
	bt_gatt_service_register(&fota_svc);

	bt_conn_cb_register(&fota_conn_callbacks);

	size_t gatt_size = ARRAY_SIZE(fota_attrs);
	fota.status_index =
		lbt_find_gatt_index(&FOTA_STATUS.uuid, fota_attrs, gatt_size);
	fota.count_index =
		lbt_find_gatt_index(&FOTA_COUNT.uuid, fota_attrs, gatt_size);
	fota.size_index =
		lbt_find_gatt_index(&FOTA_SIZE.uuid, fota_attrs, gatt_size);
	fota.file_name_index = lbt_find_gatt_index(&FOTA_FILE_NAME.uuid,
						   fota_attrs, gatt_size);
	fota.hash_index =
		lbt_find_gatt_index(&FOTA_HASH.uuid, fota_attrs, gatt_size);

	k_work_q_start(&fota.work_q, fcs_workq_stack,
		       K_THREAD_STACK_SIZEOF(fcs_workq_stack),
		       FOTA_WORKQ_THREAD_PRIORITY);

	/* The system q isn't used because fota can take a long time */
	k_work_init(&fota.work, workq_fota_handler);
}

void fota_state_handler(uint8_t state)
{
#if defined(CONFIG_MODEM_HL7800) && defined(CONFIG_MODEM_HL7800_FW_UPDATE)
	/* Map modem FOTA states to BLE states and handle state 'semaphore'. */
	if ((fota.control_point == FOTA_MODEM_START) &&
	    (fota.status == FOTA_STATUS_BUSY)) {
		switch (state) {
		case HL7800_FOTA_IDLE:
			fota_set_status(FOTA_STATUS_SUCCESS);
			break;
		case HL7800_FOTA_COMPLETE:
			fota_set_status(FOTA_STATUS_SUCCESS);
			break;
		case HL7800_FOTA_FILE_ERROR:
			LOG_ERR("FOTA File Error");
			fota_set_status(-ENOENT);
			break;
		default:
			/* Keep indicating busy. */
			break;
		}
	}
#endif
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
/* The weak implementation can be used for single peripheral designs. */
__weak struct bt_conn *fota_get_conn(void)
{
	return fota_conn;
}

static void fota_notify(bool notify, uint16_t index, uint16_t length)
{
	struct bt_conn *connection_handle = fota_get_conn();
	if (connection_handle != NULL) {
		if (notify) {
			bt_gatt_notify(connection_handle,
				       &fota_svc.attrs[index],
				       fota_svc.attrs[index].user_data, length);
		}
	}
}

static void fota_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		return;
	}

	if (!lbt_peripheral_role(conn)) {
		return;
	}

	fota_conn = bt_conn_ref(conn);
}

static void fota_disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (!lbt_peripheral_role(conn)) {
		return;
	}

	if (fota_conn) {
		bt_conn_unref(fota_conn);
		fota_conn = NULL;
	}
}

static ssize_t write_control_point(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len,
				   uint16_t offset, uint8_t flags)
{
	/* Only one command can be in process at a time. */
	if (fota.status == FOTA_STATUS_BUSY) {
		return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
	}

	ssize_t length = lbt_write_u8(conn, attr, buf, len, offset, flags);
	if (length > 0) {
		fota_set_status(FOTA_STATUS_BUSY);
		k_work_submit_to_queue(&fota.work_q, &fota.work);
	}
	return length;
}

static ssize_t read_file_name(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	return lbt_read_string(conn, attr, buf, len, offset,
			       CONFIG_FOTA_FILE_NAME_MAX_SIZE);
}

static ssize_t write_file_name(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, const void *buf,
			       uint16_t len, uint16_t offset, uint8_t flags)
{
	return lbt_write_string(conn, attr, buf, len, offset, flags,
				CONFIG_FOTA_FILE_NAME_MAX_SIZE);
}

static ssize_t read_hash(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	return lbt_read_string(conn, attr, buf, len, offset, FSU_HASH_SIZE);
}

static void status_ccc_handler(const struct bt_gatt_attr *attr, uint16_t value)
{
	ccc.status.notify = IS_NOTIFIABLE(value);
}

static void count_ccc_handler(const struct bt_gatt_attr *attr, uint16_t value)
{
	ccc.count.notify = IS_NOTIFIABLE(value);
}

static void size_ccc_handler(const struct bt_gatt_attr *attr, uint16_t value)
{
	ccc.size.notify = IS_NOTIFIABLE(value);
}

static void file_name_ccc_handler(const struct bt_gatt_attr *attr,
				  uint16_t value)
{
	ccc.file_name.notify = IS_NOTIFIABLE(value);
}

static void hash_ccc_handler(const struct bt_gatt_attr *attr, uint16_t value)
{
	ccc.hash.notify = IS_NOTIFIABLE(value);
}

/* The workqueue is used so that the BLE write isn't blocked.
 * The control point is used like a semaphore.
 */
static void workq_fota_handler(struct k_work *item)
{
	struct fota *pService = CONTAINER_OF(item, struct fota, work);
	switch (pService->control_point) {
	case FOTA_NOP:
		fota_set_status(FOTA_STATUS_SUCCESS);
		break;
	case FOTA_LIST_FILES:
		fota_list_files(pService->file_name);
		break;
	case FOTA_MODEM_START:
		fota_modem_start(pService->file_name);
		break;
	case FOTA_DELETE_FILES:
		fota_delete_files(pService->file_name);
		break;
	case FOTA_COMPUTE_SHA256:
		fota_compute_sha256(pService->file_name);
		break;
	}
}

static void fota_list_files(const char *name)
{
	size_t count = 0;
	struct fs_dirent *pEntries =
		fsu_find(CONFIG_FOTA_FS_MOUNT, name, &count, FS_DIR_ENTRY_FILE);
	size_t i;
	for (i = 0; i < count; i++) {
		fota_set_file_name(pEntries[i].name);
		fota_set_size(pEntries[i].size);
	}
	fsu_free_found(pEntries);
	if (count == 0) {
		fota_set_status(-ENOENT);
	} else {
		fota_set_status(FOTA_STATUS_SUCCESS);
	}
}

static void fota_modem_start(const char *name)
{
	int status = FOTA_STATUS_SUCCESS;
	/* Ensure file name only matches one file and that it exists. */
	size_t count = 0;
	struct fs_dirent *pEntries =
		fsu_find(CONFIG_FOTA_FS_MOUNT, name, &count, FS_DIR_ENTRY_FILE);
	if (count == 0) {
		status = -ENOENT;
		fota_set_status(status);
	} else if (count != 1) {
		status = -EPERM;
		fota_set_status(status);
	} else {
		fota_set_size(pEntries[0].size);
		fota_set_count(0);
	}
	fsu_free_found(pEntries);

	/* Pass control to modem task. */
	if (status == FOTA_STATUS_SUCCESS) {
		fota_build_full_name(pEntries[0].name);
		LOG_DBG("Requesting modem FOTA with file %s",
			log_strdup(fota_full_name));
#if CONFIG_MODEM_HL7800_FW_UPDATE
		status = mdm_hl7800_update_fw(fota_full_name);
#else
		status = FOTA_STATUS_ERROR;
#endif
		if (status != 0) {
			fota_set_status(FOTA_STATUS_ERROR);
			LOG_ERR("FOTA failed to start");
		}
	}
}

static void fota_delete_files(const char *name)
{
	int status = FOTA_STATUS_SUCCESS;
	size_t count = 0;
	struct fs_dirent *pEntries =
		fsu_find(CONFIG_FOTA_FS_MOUNT, name, &count, FS_DIR_ENTRY_FILE);
	if (count == 0) {
		status = -ENOENT;
	} else {
		size_t i = 0;
		while ((i < count) && (status == FOTA_STATUS_SUCCESS)) {
			fota_build_full_name(pEntries[i].name);
			LOG_DBG("Deleting (unlinking) file %s",
				log_strdup(fota_full_name));
			status = fs_unlink(fota_full_name);
			i += 1;
		}
	}
	fota_set_status(status);
	fsu_free_found(pEntries);
}

static void fota_compute_sha256(const char *name)
{
	uint8_t hash[FSU_HASH_SIZE];
	ssize_t status = FOTA_STATUS_SUCCESS;
	size_t count = 0;
	struct fs_dirent *pEntries =
		fsu_find(CONFIG_FOTA_FS_MOUNT, name, &count, FS_DIR_ENTRY_FILE);
	if (count == 0) {
		status = -ENOENT;
	} else {
		size_t i = 0;
		while ((i < count) && (status == FOTA_STATUS_SUCCESS)) {
			LOG_DBG("Computing hash for %s",
				log_strdup(pEntries[i].name));
			status = fsu_sha256(hash, CONFIG_FOTA_FS_MOUNT,
					    pEntries[i].name, pEntries[i].size);
			if (status == FOTA_STATUS_SUCCESS) {
				fota_set_hash(hash);
			}
			i += 1;
		}
	}
	fota_set_status((int)status);
	fsu_free_found(pEntries);
}

static void fota_build_full_name(const char *name)
{
	fsu_build_full_name(fota_full_name, sizeof(fota_full_name),
			    CONFIG_FOTA_FS_MOUNT, name);
}
