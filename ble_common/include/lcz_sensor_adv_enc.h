/**
 * @file lcz_sensor_adv_enc.h
 * @brief Support functions for encrypting/decrypting advertisement data
 *
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: LicenseRef-LairdConnectivity-Clause
 */
#ifndef __LCZ_SENSOR_ADV_ENC_H__
#define __LCZ_SENSOR_ADV_ENC_H__

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <zephyr/types.h>
#include "lcz_sensor_adv_format.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************/
/* Global Function Prototypes                                                                     */
/**************************************************************************************************/
#if defined(CONFIG_LCZ_PKI_AUTH_SMP_PERIPHERAL)
/**
 * @brief Checks to see if advertisements can be encrypted
 *
 * @returns true if advertisements can be encrypted, false if not
 */
bool lcz_sensor_adv_can_encrypt(void);

/**
 * @brief Encrypt an advertisement
 *
 * @param ad Advertisement data to encrypt
 *
 * @return 0 on success, <0 on error
 */
int lcz_sensor_adv_encrypt(LczSensorDMEncrAd_t *ad);
#endif

#if defined(CONFIG_LCZ_PKI_AUTH_SMP_CENTRAL)
/**
 * @brief Decrypt an advertisement
 *
 * @param addr Pointer to BLE address of the advertiser
 * @param ad Advertisement data to decrypt
 *
 * @return 0 on success, <0 on error
 */
int lcz_sensor_adv_decrypt(const bt_addr_le_t *addr, LczSensorDMEncrAd_t *ad);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_SENSOR_ADV_ENC_H__ */
