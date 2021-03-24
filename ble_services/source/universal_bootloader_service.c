/**
 * @file universal_bootloader_service.c
 * @brief BLE Universal Bootloader Service
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(universal_bootloader_service);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/bluetooth.h>

#include "lcz_bluetooth.h"
#include "universal_bootloader_service.h"
#include "Bootloader_External_Settings.h"
#include "hexcode.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define UBS_BASE_UUID_128(_x_)                                                 \
	BT_UUID_INIT_128(0xa0, 0xe3, 0x4f, 0x84, 0xb8, 0x2c, 0x04, 0xd3, 0xe0, \
			 0xf5, 0x7a, 0x7a, LSB_16(_x_), MSB_16(_x_), 0x2b,     \
			 0xe5)

/* clang-format off */
static struct bt_uuid_128 UBS_UUID = UBS_BASE_UUID_128(0x0000);
static struct bt_uuid_128 BOOTLOADER_PRESENT_UUID = UBS_BASE_UUID_128(0x0001);
static struct bt_uuid_128 BOOTLOADER_HEADER_CHECKED_UUID = UBS_BASE_UUID_128(0x0002);
static struct bt_uuid_128 ERROR_CODE_UUID = UBS_BASE_UUID_128(0x0003);
static struct bt_uuid_128 BOOTLOADER_VERSION_UUID = UBS_BASE_UUID_128(0x0004);
static struct bt_uuid_128 EXT_HEADER_VERSION_UUID = UBS_BASE_UUID_128(0x0005);
static struct bt_uuid_128 EXT_FUNCTION_VERSION_UUID = UBS_BASE_UUID_128(0x0006);
static struct bt_uuid_128 CUSTOMER_KEY_SET_UUID = UBS_BASE_UUID_128(0x0007);
static struct bt_uuid_128 CUSTOMER_KEY_UUID = UBS_BASE_UUID_128(0x0008);
static struct bt_uuid_128 READBACK_PROTECTION_UUID = UBS_BASE_UUID_128(0x0009);
static struct bt_uuid_128 CPU_DEBUG_PROTECTION_UUID = UBS_BASE_UUID_128(0x000a);
static struct bt_uuid_128 QSPI_CHECKED_UUID = UBS_BASE_UUID_128(0x000b);
static struct bt_uuid_128 QSPI_CRC_UUID = UBS_BASE_UUID_128(0x000c);
static struct bt_uuid_128 QSPI_SHA256_UUID = UBS_BASE_UUID_128(0x000d);
static struct bt_uuid_128 BOOTLOADER_TYPE_UUID = UBS_BASE_UUID_128(0x000e);
static struct bt_uuid_128 BOOTLOADER_UPDATE_FAILURES_UUID = UBS_BASE_UUID_128(0x000f);
static struct bt_uuid_128 BOOTLOADER_UPDATE_LAST_FAIL_VERSION_UUID = UBS_BASE_UUID_128(0x0010);
static struct bt_uuid_128 BOOTLOADER_UPDATE_LAST_FAIL_CODE_UUID = UBS_BASE_UUID_128(0x0011);
static struct bt_uuid_128 BOOTLOADER_UPDATES_APPLIED_UUID = UBS_BASE_UUID_128(0x0012);
static struct bt_uuid_128 BOOTLOADER_SECTION_UPDATES_APPLIED_UUID = UBS_BASE_UUID_128(0x0013);
static struct bt_uuid_128 BOOTLOADER_MODEM_UPDATES_APPLIED_UUID = UBS_BASE_UUID_128(0x0014);
static struct bt_uuid_128 BOOTLOADER_MODEM_UPDATE_LAST_FAIL_VERSION_UUID = UBS_BASE_UUID_128(0x0015);
static struct bt_uuid_128 BOOTLOADER_MODEM_UPDATE_LAST_FAIL_CODE_UUID = UBS_BASE_UUID_128(0x0016);
static struct bt_uuid_128 BOOTLOADER_COMPRESSION_ERRORS_UUID = UBS_BASE_UUID_128(0x0017);
static struct bt_uuid_128 BOOTLOADER_COMPRESSION_LAST_FAIL_CODE_UUID = UBS_BASE_UUID_128(0x0018);
static struct bt_uuid_128 MODULE_BUILD_DATE_UUID = UBS_BASE_UUID_128(0x0019);
static struct bt_uuid_128 FIRMWARE_BUILD_DATE_UUID = UBS_BASE_UUID_128(0x001a);
static struct bt_uuid_128 BOOT_VERIFICATION_UUID = UBS_BASE_UUID_128(0x001b);
/* clang-format on */

struct universal_bootloader_service {
	bool bootloader_present;
	bool bootloader_header_checked;
	uint8_t error_code;
	uint16_t bootloader_version;
	uint16_t ext_header_version;
	uint16_t ext_function_version;
	bool customer_key_set;
	char customer_key[SIGNATURE_SIZE * 2 + 1];
	bool readback_protection;
	bool cpu_debug_protection;
	uint8_t QSPI_checked;
	uint32_t QSPI_crc;
	char QSPI_sha256[CONFIG_SHA256_SIZE * 2 + 1];
	bool bootloader_type;
	uint8_t bootloader_update_failures;
	uint16_t bootloader_update_last_fail_version;
	uint8_t bootloader_update_last_fail_code;
	uint16_t bootloader_updates_applied;
	uint16_t bootloader_section_updates_applied;
	uint16_t bootloader_modem_updates_applied;
	uint16_t bootloader_modem_update_last_fail_version;
	uint8_t bootloader_modem_update_last_fail_code;
	uint8_t bootloader_compression_errors;
	uint16_t bootloader_compression_last_fail_code;
	char module_build_date[sizeof(__DATE__)];
	char firmware_build_date[sizeof(__DATE__)];
	uint8_t boot_verification;
};

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static struct universal_bootloader_service ubs;

/******************************************************************************/
/* Bootloader Service Declaration                                             */
/******************************************************************************/
static struct bt_gatt_attr bootloader_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(&UBS_UUID),

	BT_GATT_CHARACTERISTIC(&BOOTLOADER_PRESENT_UUID.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, lbt_read_u8, NULL,
			       &ubs.bootloader_present),

	BT_GATT_CHARACTERISTIC(&BOOTLOADER_HEADER_CHECKED_UUID.uuid,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       lbt_read_u8, NULL,
			       &ubs.bootloader_header_checked),

	BT_GATT_CHARACTERISTIC(&ERROR_CODE_UUID.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, lbt_read_u8, NULL,
			       &ubs.error_code),

	BT_GATT_CHARACTERISTIC(&BOOTLOADER_VERSION_UUID.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, lbt_read_u16, NULL,
			       &ubs.bootloader_version),

	BT_GATT_CHARACTERISTIC(&EXT_HEADER_VERSION_UUID.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, lbt_read_u16, NULL,
			       &ubs.ext_header_version),

	BT_GATT_CHARACTERISTIC(&EXT_FUNCTION_VERSION_UUID.uuid,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       lbt_read_u16, NULL, &ubs.ext_function_version),

	BT_GATT_CHARACTERISTIC(&CUSTOMER_KEY_SET_UUID.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, lbt_read_u8, NULL,
			       &ubs.customer_key_set),

	BT_GATT_CHARACTERISTIC(&CUSTOMER_KEY_UUID.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, lbt_read_string_no_max_size,
			       NULL, &ubs.customer_key),

	BT_GATT_CHARACTERISTIC(&READBACK_PROTECTION_UUID.uuid,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       lbt_read_u8, NULL, &ubs.readback_protection),

	BT_GATT_CHARACTERISTIC(&CPU_DEBUG_PROTECTION_UUID.uuid,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       lbt_read_u8, NULL, &ubs.cpu_debug_protection),

	BT_GATT_CHARACTERISTIC(&QSPI_CHECKED_UUID.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, lbt_read_u8, NULL,
			       &ubs.QSPI_checked),

	BT_GATT_CHARACTERISTIC(&QSPI_CRC_UUID.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, lbt_read_u32, NULL,
			       &ubs.QSPI_crc),

	BT_GATT_CHARACTERISTIC(&QSPI_SHA256_UUID.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, lbt_read_string_no_max_size,
			       NULL, &ubs.QSPI_sha256),

	BT_GATT_CHARACTERISTIC(&BOOTLOADER_TYPE_UUID.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, lbt_read_u8, NULL,
			       &ubs.bootloader_type),

	BT_GATT_CHARACTERISTIC(&BOOTLOADER_UPDATE_FAILURES_UUID.uuid,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       lbt_read_u8, NULL,
			       &ubs.bootloader_update_failures),

	BT_GATT_CHARACTERISTIC(&BOOTLOADER_UPDATE_LAST_FAIL_VERSION_UUID.uuid,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       lbt_read_u16, NULL,
			       &ubs.bootloader_update_last_fail_version),

	BT_GATT_CHARACTERISTIC(&BOOTLOADER_UPDATE_LAST_FAIL_CODE_UUID.uuid,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       lbt_read_u8, NULL,
			       &ubs.bootloader_update_last_fail_code),

	BT_GATT_CHARACTERISTIC(&BOOTLOADER_UPDATES_APPLIED_UUID.uuid,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       lbt_read_u16, NULL,
			       &ubs.bootloader_updates_applied),

	BT_GATT_CHARACTERISTIC(&BOOTLOADER_SECTION_UPDATES_APPLIED_UUID.uuid,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       lbt_read_u16, NULL,
			       &ubs.bootloader_section_updates_applied),

	BT_GATT_CHARACTERISTIC(&BOOTLOADER_MODEM_UPDATES_APPLIED_UUID.uuid,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       lbt_read_u16, NULL,
			       &ubs.bootloader_modem_updates_applied),

	BT_GATT_CHARACTERISTIC(
		&BOOTLOADER_MODEM_UPDATE_LAST_FAIL_VERSION_UUID.uuid,
		BT_GATT_CHRC_READ, BT_GATT_PERM_READ, lbt_read_u16, NULL,
		&ubs.bootloader_modem_update_last_fail_version),

	BT_GATT_CHARACTERISTIC(
		&BOOTLOADER_MODEM_UPDATE_LAST_FAIL_CODE_UUID.uuid,
		BT_GATT_CHRC_READ, BT_GATT_PERM_READ, lbt_read_u8, NULL,
		&ubs.bootloader_modem_update_last_fail_code),

	BT_GATT_CHARACTERISTIC(&BOOTLOADER_COMPRESSION_ERRORS_UUID.uuid,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       lbt_read_u8, NULL,
			       &ubs.bootloader_compression_errors),

	BT_GATT_CHARACTERISTIC(&BOOTLOADER_COMPRESSION_LAST_FAIL_CODE_UUID.uuid,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       lbt_read_u16, NULL,
			       &ubs.bootloader_compression_last_fail_code),

	BT_GATT_CHARACTERISTIC(&MODULE_BUILD_DATE_UUID.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, lbt_read_string_no_max_size,
			       NULL, &ubs.module_build_date),

	BT_GATT_CHARACTERISTIC(&FIRMWARE_BUILD_DATE_UUID.uuid,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       lbt_read_string_no_max_size, NULL,
			       &ubs.firmware_build_date),

	BT_GATT_CHARACTERISTIC(&BOOT_VERIFICATION_UUID.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, lbt_read_u8, NULL,
			       &ubs.boot_verification)
};

static struct bt_gatt_service bootloader_service =
	BT_GATT_SERVICE(bootloader_attrs);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void ubs_set_bootloader_present(bool present)
{
	ubs.bootloader_present = present;
}

void ubs_set_bootloader_header_checked(bool checked)
{
	ubs.bootloader_header_checked = checked;
}

void ubs_set_error_code(uint8_t error)
{
	ubs.error_code = error;
}

void ubs_set_bootloader_version(uint16_t version)
{
	ubs.bootloader_version = version;
}

void ubs_set_ext_header_version(uint16_t version)
{
	ubs.ext_header_version = version;
}

void ubs_set_ext_function_version(uint16_t version)
{
	ubs.ext_function_version = version;
}

void ubs_set_customer_key_set(bool set)
{
	ubs.customer_key_set = set;
}

void ubs_set_customer_key(uint8_t *key)
{
	if (key == NULL) {
		memset(ubs.customer_key, 0, sizeof(ubs.customer_key));
	} else {
		HexEncode(key, SIGNATURE_SIZE * 2, ubs.customer_key, false,
			  true);
	}
}

void ubs_set_readback_protection(bool readback)
{
	ubs.readback_protection = readback;
}

void ubs_set_cpu_debug_protection(bool debug)
{
	ubs.cpu_debug_protection = debug;
}

void ubs_set_QSPI_checked(uint8_t checked)
{
	ubs.QSPI_checked = checked;
}

void ubs_set_QSPI_crc(uint32_t checked)
{
	ubs.QSPI_crc = checked;
}

void ubs_set_QSPI_sha256(uint8_t *sha256)
{
	if (sha256 == NULL) {
		memset(ubs.QSPI_sha256, 0, sizeof(ubs.QSPI_sha256));
	} else {
		HexEncode(sha256, CONFIG_SHA256_SIZE * 2, ubs.QSPI_sha256,
			  false, true);
	}
}

void ubs_set_bootloader_type(bool type)
{
	ubs.bootloader_type = type;
}

void ubs_set_bootloader_update_failures(uint8_t failures)
{
	ubs.bootloader_update_failures = failures;
}

void ubs_set_bootloader_update_last_fail_version(uint16_t version)
{
	ubs.bootloader_update_last_fail_version = version;
}

void ubs_set_bootloader_update_last_fail_code(uint8_t code)
{
	ubs.bootloader_update_last_fail_code = code;
}

void ubs_set_bootloader_updates_applied(uint16_t updates)
{
	ubs.bootloader_updates_applied = updates;
}

void ubs_set_bootloader_section_updates_applied(uint16_t updates)
{
	ubs.bootloader_section_updates_applied = updates;
}

void ubs_set_bootloader_modem_updates_applied(uint16_t updates)
{
	ubs.bootloader_modem_updates_applied = updates;
}

void ubs_set_bootloader_modem_update_last_fail_version(uint16_t version)
{
	ubs.bootloader_modem_update_last_fail_version = version;
}

void ubs_set_bootloader_modem_update_last_fail_code(uint8_t code)
{
	ubs.bootloader_modem_update_last_fail_code = code;
}

void ubs_set_bootloader_compression_errors(uint8_t errors)
{
	ubs.bootloader_compression_errors = errors;
}

void ubs_set_bootloader_compression_last_fail_code(uint16_t code)
{
	ubs.bootloader_compression_last_fail_code = code;
}

void ubs_set_module_build_date(uint8_t *date)
{
	if (date == NULL) {
		memset(ubs.module_build_date, 0, sizeof(ubs.module_build_date));
	} else {
		memcpy(ubs.module_build_date, date,
		       sizeof(ubs.module_build_date));
	}
}

void ubs_set_firmware_build_date(uint8_t *date)
{
	if (date == NULL) {
		memset(ubs.firmware_build_date, 0,
		       sizeof(ubs.firmware_build_date));
	} else {
		memcpy(ubs.firmware_build_date, date,
		       sizeof(ubs.firmware_build_date));
	}
}

void ubs_set_boot_verification(uint8_t verification)
{
	ubs.boot_verification = verification;
}

void ubs_init()
{
	bt_gatt_service_register(&bootloader_service);

	ubs_set_customer_key(NULL);
	ubs_set_QSPI_sha256(NULL);
	ubs_set_module_build_date(NULL);
	ubs_set_firmware_build_date(NULL);
}
