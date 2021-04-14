/**
 * @file ble_ess_service.h
 * @brief Provides a BLE environmental sensing service
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BLE_ESS_SERVICE_H__
#define __BLE_ESS_SERVICE_H__

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <bluetooth/conn.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* Global Constants, Macros and Type Definitions                              */
/******************************************************************************/
enum ess_types_t {
	ESS_TYPE_TEMPERATURE = 0,
	ESS_TYPE_HUMIDITY,
	ESS_TYPE_PRESSURE,
	ESS_TYPE_DEW_POINT,

	ESS_TYPE_MAX_COUNT
};

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
/* For multi-peripheral device the weak implementation can be overriden. */
struct bt_conn *ess_svc_get_conn(void);

/** @brief Callback when ESS notifications are enabled or disabled
 *         This weak implementation can be overridden by applications that need
 *         it
 */
void ess_notifications_changed(enum ess_types_t sensor, bool enabled);

/** @brief Converts an ESS type to a string
 *
 *  @param [in]sensor - The sensor type
 *  @return String of sensor type or ERROR
 */
char *ess_type_to_string(enum ess_types_t sensor);

/** @brief Initialises the ESS service, must be called before use
 */
void ess_svc_init();

/** @brief Updates the temperature of the ESS (and sends a notification if
 *         enabled). Temperature format is in degrees celsius in 0.01 units
 *         (e.g. value of 391 is 3.91c)
 *
 *  @param [in]conn - Connection handle to notify (or NULL)
 *  @param [in]value - new temperature value
 */
void ess_svc_update_temperature(struct bt_conn *conn, int16_t value);

/** @brief Updates the humidity of the ESS (and sends a notification if
 *         enabled). Humidity format is in percent in 0.01 units (e.g. value
 *         of 524 is 5.24%)
 *
 *  @param [in]conn - Connection handle to notify (or NULL)
 *  @param [in]value - new humidity value
 */
void ess_svc_update_humidity(struct bt_conn *conn, int16_t value);

/** @brief Updates the pressure of the ESS (and sends a notification if
 *         enabled). Pressure format is in pascals in 0.1 units (e.g. value of
 *         86 is 8.6Pa)
 *
 *  @param [in]conn - Connection handle to notify (or NULL)
 *  @param [in]value - new pressure value
 */
void ess_svc_update_pressure(struct bt_conn *conn, int32_t value);

/** @brief Updates the dew point of the ESS (and sends a notification if
 *         enabled). Dew point format is in degress celcius in 1 units (e.g.
 *         value of 5 is 5c)
 *
 *  @param [in]conn - Connection handle to notify (or NULL)
 *  @param [in]value - new dew point value
 */
void ess_svc_update_dew_point(struct bt_conn *conn, int8_t value);

#ifdef __cplusplus
}
#endif

#endif /* __BLE_ESS_SERVICE_H__ */
