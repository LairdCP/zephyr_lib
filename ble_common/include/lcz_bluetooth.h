/**
 * @file lcz_bluetooth.h
 * @brief Bluetooth helper functions
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LCZ_BLUETOOTH_H__
#define __LCZ_BLUETOOTH_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <bluetooth/gatt.h>
#include <bluetooth/bluetooth.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Constants, Macros and Type Definitions                              */
/******************************************************************************/
#define IS_NOTIFIABLE(v) ((v) == BT_GATT_CCC_NOTIFY) ? true : false;

/** The upper 8 bits of a 16 bit value */
#define MSB_16(a) (((a)&0xFF00) >> 8)
/** The lower 8 bits (of a 16 bit value) */
#define LSB_16(a) ((a)&0x00FF)

/* Client Characteristic Configuration Descriptor */
struct lbt_ccc_element {
	bool notify;
};

/* Link a handler with the form name_ccc_handler  */
#define LBT_GATT_CCC(name)                                                     \
	BT_GATT_CCC(name##_ccc_handler, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)

/* The next handle after a service UUID */
#define LBT_NEXT_HANDLE_AFTER_SERVICE(x) ((x) + 1)

/* The next (possible) handle after a characterstic (UUID and value) */
#define LBT_NEXT_HANDLE_AFTER_CHAR(x) ((x) + 2)

#define BT_SUCCESS 0

/** ATT_MTU - OpCode (1 byte) - Handle (2 bytes) */
#define BT_MAX_PAYLOAD(x) ((x)-3)

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 *  @brief Helper function for reading a byte from the Bluetooth stack.
 */
ssize_t lbt_read_u8(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		    void *buf, uint16_t len, uint16_t offset);

/**
 *  @brief Helper function for reading a uint16 from the Bluetooth stack.
 */
ssize_t lbt_read_u16(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		     void *buf, uint16_t len, uint16_t offset);

/**
 *  @brief Helper function for reading a uint32 from the Bluetooth stack.
 */
ssize_t lbt_read_u32(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		     void *buf, uint16_t len, uint16_t offset);

/**
 *  @brief Helper function for reading an integer from the Bluetooth stack.
 */
ssize_t lbt_read_integer(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset);

/**
 *  @brief Helper function for reading a string characteristic from
 * the Bluetooth stack.
 */
ssize_t lbt_read_string(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset,
			uint16_t max_str_length);

/**
 *  @brief Helper function for reading a string characteristic from
 * the Bluetooth stack (size of full attribute will be used).
 */
ssize_t lbt_read_string_no_max_size(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr, void *buf,
				    uint16_t len, uint16_t offset);

/**
 * @brief Helper function for writing a string characteristic from the
 * Bluetooth stack.
 * @param max_str_length is the maximum string length of the item.
 * It is assumed by this function the the actual size of the item is 1 character
 * longer so that the string can be NULL terminated.
 */
ssize_t lbt_write_string(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags, uint16_t max_str_length);

/**
 * @brief Helper function for writing a uint8_t from the Bluetooth stack.
 */
ssize_t lbt_write_u8(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		     const void *buf, uint16_t len, uint16_t offset,
		     uint8_t flags);

/**
 * @brief Helper function for writing a uint16_t from the Bluetooth stack.
 */
ssize_t lbt_write_u16(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		      const void *buf, uint16_t len, uint16_t offset,
		      uint8_t flags);

/**
 * @brief Helper function for finding a characteristic handle in the GATT
 * table.
 */
uint16_t lbt_find_gatt_index(struct bt_uuid *uuid, struct bt_gatt_attr *gatt,
			     size_t size);

/**
 * @retval ATT error code as a string.
 */
const char *lbt_get_att_err_string(uint8_t code);

/**
 * @retval HCI error code as a string.
 */
const char *lbt_get_hci_err_string(uint8_t code);

/**
 * @retval Reset reason code
 */
uint32_t lbt_get_and_clear_nrf52_reset_reason_register(void);

/**
 * @brief The reset reason register can have multiple bits set.
 * The reset reason should be cleared after it is used to prevent this.
 * This function returns the code for the first bit that is set in its
 * prioritized list.
 *
 * @retval Reset reason code as a string (from reset reason register).
 */
const char *lbt_get_nrf52_reset_reason_string_from_register(uint32_t reg);

/**
 * @retval true if central role, false if peripheral role or there
 * was an error getting connection info.
 */
bool lbt_central_role(struct bt_conn *conn);

/**
 * @retval true if peripheral role, false if central role or there
 * was an error getting connection info.
 */
bool lbt_peripheral_role(struct bt_conn *conn);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_BLUETOOTH_H__ */
