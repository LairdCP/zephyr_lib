/**
 * @file universal_bootloader_service.h
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __UNIVERSAL_BOOTLOADER_SERVICE_H__
#define __UNIVERSAL_BOOTLOADER_SERVICE_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <bluetooth/conn.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
/**
 * @brief Initialise the bootloader service
 */
void ubs_init();

/**
 * @brief Various functions for setting bootloader service data
 */
void ubs_set_bootloader_present(bool present);
void ubs_set_bootloader_state(uint8_t state);
void ubs_set_bootloader_header_checked(bool checked);
void ubs_set_error_code(uint8_t error);
void ubs_set_bootloader_version(uint16_t version);
void ubs_set_bootloader_build_date(uint8_t *date);
void ubs_set_ext_function_version(uint16_t version);
void ubs_set_ext_header_version(uint16_t version);
void ubs_set_customer_key_set(bool set);
void ubs_set_customer_key(uint8_t *key);
void ubs_set_readback_protection(bool readback);
void ubs_set_cpu_debug_protection(bool debug);
void ubs_set_QSPI_checked(uint8_t checked);
void ubs_set_QSPI_crc(uint32_t checked);
void ubs_set_QSPI_sha256(uint8_t *sha256);
void ubs_set_bootloader_type(bool type);
void ubs_set_bootloader_update_failures(uint8_t failures);
void ubs_set_bootloader_update_last_fail_version(uint16_t version);
void ubs_set_bootloader_update_last_fail_code(uint8_t code);
void ubs_set_bootloader_updates_applied(uint16_t updates);
void ubs_set_bootloader_section_updates_applied(uint16_t updates);
void ubs_set_bootloader_modem_updates_applied(uint16_t updates);
void ubs_set_bootloader_modem_update_last_fail_version(uint16_t version);
void ubs_set_bootloader_modem_update_last_fail_code(uint8_t code);
void ubs_set_bootloader_compression_errors(uint8_t errors);
void ubs_set_bootloader_compression_last_fail_code(uint16_t code);
void ubs_set_module_build_date(uint8_t *date);
void ubs_set_firmware_build_date(uint8_t *date);
void ubs_set_boot_verification(uint8_t verification);

#ifdef __cplusplus
}
#endif

#endif /* __UNIVERSAL_BOOTLOADER_SERVICE_H__ */
