/**
 * @file lcz_sensor_adv_enc.c
 * @brief Support functions for encrypting/decrypting advertisement data
 *
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: LicenseRef-LairdConnectivity-Clause
 */
/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_adv_enc, CONFIG_LCZ_SENSOR_ADV_ENC_LOG_LEVEL);

#include <zephyr.h>
#include <stdint.h>
#include "psa/crypto.h"
#include "psa/error.h"

#include "lcz_pki_auth_smp.h"
#include "lcz_sensor_adv_enc.h"

/**************************************************************************************************/
/* Global Constant, Macro and Type Definitions                                                    */
/**************************************************************************************************/
#define ENC_BLOCK_SIZE 16

#define ENC_BLOCK_0_CONST 0xD6
#define ENC_BLOCK_9_CONST 0x00

/**************************************************************************************************/
/* Local Function Prototypes                                                                      */
/**************************************************************************************************/
#if (defined(CONFIG_LCZ_PKI_AUTH_SMP_PERIPHERAL) || defined(CONFIG_LCZ_PKI_AUTH_SMP_CENTRAL))
static int encrypt_decrypt(LczSensorDMEncrAd_t *ad, psa_key_id_t enc_key);
static int mic_compute(LczSensorDMEncrAd_t *ad, uint8_t *mic, psa_key_id_t sig_key);
#endif

/**************************************************************************************************/
/* Local Data Definitions                                                                         */
/**************************************************************************************************/

/**************************************************************************************************/
/* Local Function Definitions                                                                     */
/**************************************************************************************************/
#if (defined(CONFIG_LCZ_PKI_AUTH_SMP_PERIPHERAL) || defined(CONFIG_LCZ_PKI_AUTH_SMP_CENTRAL))
static int encrypt_decrypt(LczSensorDMEncrAd_t *ad, psa_key_id_t enc_key)
{
	uint8_t in_block[ENC_BLOCK_SIZE];
	uint8_t out_block[ENC_BLOCK_SIZE];
	size_t out_size;
	int err;

	/* Build the block */
	in_block[0] = ENC_BLOCK_0_CONST;
	in_block[1] = ad->addr.val[0];
	in_block[2] = ad->addr.val[1];
	in_block[3] = ad->addr.val[2];
	in_block[4] = ad->addr.val[3];
	in_block[5] = ad->addr.val[4];
	in_block[6] = ad->addr.val[5];
	in_block[7] = (ad->id >> 8) & 0xFF;
	in_block[8] = (ad->id >> 0) & 0xFF;
	in_block[9] = ENC_BLOCK_9_CONST;
	in_block[10] = (ad->epoch >> 24) & 0xFF;
	in_block[11] = (ad->epoch >> 16) & 0xFF;
	in_block[12] = (ad->epoch >> 8) & 0xFF;
	in_block[13] = (ad->epoch >> 0) & 0xFF;
	in_block[14] = (ad->networkId >> 8) & 0xFF;
	in_block[15] = (ad->networkId >> 0) & 0xFF;

	/* Encrypt the block */
	err = psa_cipher_encrypt(enc_key, LCZ_PKI_AUTH_SMP_SESSION_ENC_KEY_ALG, in_block,
				 sizeof(in_block), out_block, sizeof(out_block), &out_size);
	if (err != PSA_SUCCESS) {
		LOG_ERR("encrypt_decrypt: failed %d", err);
	} else if (out_size != sizeof(out_block)) {
		err = -ENODATA;
		LOG_ERR("encrypt_decrypt: output size error (expected %d, got %d)",
			sizeof(out_block), out_size);
	}

	/* Encrypt/decrypt the advertisement data */
	if (err == PSA_SUCCESS) {
		ad->recordType ^= out_block[0];
		ad->data.u32 = ((((ad->data.u32 >> 24) & 0xFF) ^ out_block[1]) << 24) |
			       ((((ad->data.u32 >> 16) & 0xFF) ^ out_block[2]) << 16) |
			       ((((ad->data.u32 >> 8) & 0xFF) ^ out_block[3]) << 8) |
			       ((((ad->data.u32 >> 0) & 0xFF) ^ out_block[4]) << 0);
	}

	return err;
}

static int mic_compute(LczSensorDMEncrAd_t *ad, uint8_t *mic, psa_key_id_t sig_key)
{
	psa_mac_operation_t operation = PSA_MAC_OPERATION_INIT;
	size_t mic_size;
	uint32_t whole_mic;
	int err = 0;

	/* Set up the operation */
	err = psa_mac_sign_setup(&operation, sig_key,
				 PSA_ALG_TRUNCATED_MAC(PSA_ALG_CMAC, sizeof(whole_mic)));
	if (err != PSA_SUCCESS) {
		LOG_ERR("mic_compute: MIC setup failed: %d", err);
	}

	/* Add the first half of the advertisement (before mic) */
	if (err == PSA_SUCCESS) {
		err = psa_mac_update(&operation, (uint8_t *)ad, offsetof(LczSensorDMEncrAd_t, mic));
		if (err != PSA_SUCCESS) {
			LOG_ERR("mic_compute: Adding first half failed: %d", err);
		}
	}

	/* Add the second half of the advertisement (after mic) */
	if (err == PSA_SUCCESS) {
		err = psa_mac_update(
			&operation, ((uint8_t *)ad) + offsetof(LczSensorDMEncrAd_t, epoch),
			sizeof(LczSensorDMEncrAd_t) - offsetof(LczSensorDMEncrAd_t, epoch));
		if (err != PSA_SUCCESS) {
			LOG_ERR("mic_compute: Adding second half failed: %d", err);
		}
	}

	/* Complete the MIC calculation */
	if (err == PSA_SUCCESS) {
		err = psa_mac_sign_finish(&operation, (uint8_t *)&whole_mic, sizeof(whole_mic),
					  &mic_size);
		if (err != PSA_SUCCESS) {
			LOG_ERR("mic_compute: MIC complete failed: %d", err);
		} else if (mic_size != sizeof(whole_mic)) {
			LOG_ERR("mic_compute: MIC size error (expected %d, got %d)",
				sizeof(whole_mic), mic_size);
		} else if (mic != NULL) {
			/* Only return 2 bytes of MIC from our 4 */
			mic[0] = (whole_mic >> 24) & 0xFF;
			mic[1] = (whole_mic >> 16) & 0xFF;
		}
	}

	/* Clean up the operation */
	psa_mac_abort(&operation);

	return err;
}
#endif /* CONFIG_LCZ_PKI_AUTH_SMP_PERIPHERAL || CONFIG_LCZ_PKI_AUTH_SMP_CENTRAL */

/**************************************************************************************************/
/* Global Function Definitions                                                                    */
/**************************************************************************************************/
#if defined(CONFIG_LCZ_PKI_AUTH_SMP_PERIPHERAL)
bool lcz_sensor_adv_can_encrypt(void)
{
	/* If we can fetch keys, we can encrypt */
	if (lcz_pki_auth_smp_periph_get_keys(NULL, NULL, NULL) == 0) {
		return true;
	}
	return false;
}

int lcz_sensor_adv_encrypt(LczSensorDMEncrAd_t *ad)
{
	psa_key_id_t enc_key;
	psa_key_id_t sig_key;
	int err = 0;

	/* Fetch our keys */
	err = lcz_pki_auth_smp_periph_get_keys(NULL, &enc_key, &sig_key);
	if (err < 0) {
		LOG_ERR("lcz_sensor_adv_encrypt: failed to fetch keys: %d", err);
	}

	/* Encrypt the advertisement */
	if (err == 0) {
		err = encrypt_decrypt(ad, enc_key);
	}

	/* Compute the MIC */
	if (err == 0) {
		err = mic_compute(ad, (uint8_t *)&(ad->mic), sig_key);
	}

	return err;
}
#endif

#if defined(CONFIG_LCZ_PKI_AUTH_SMP_CENTRAL)
int lcz_sensor_adv_decrypt(const bt_addr_le_t *addr, LczSensorDMEncrAd_t *ad)
{
	psa_key_id_t enc_key;
	psa_key_id_t sig_key;
	uint16_t mic;
	int err = 0;

	/* Fetch the keys */
	err = lcz_pki_auth_smp_central_get_keys(addr, NULL, &enc_key, &sig_key);

	/* Compute the MIC */
	if (err == 0) {
		err = mic_compute(ad, (uint8_t *)&mic, sig_key);
	}

	/* Verify the MIC */
	if (err == 0) {
		if (mic != ad->mic) {
			err = -EINVAL;
		}
	}

	/* Decrypt the advertisement */
	if (err == 0) {
		err = encrypt_decrypt(ad, enc_key);
	}

	return err;
}
#endif
