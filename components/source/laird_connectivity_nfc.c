/**
 * @file laird_connectivity_nfc.c
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(lairdconnect_nfc);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <stdlib.h>
#include <zephyr.h>

#include <nfc_t2t_lib.h>
#include <nfc/ndef/msg.h>
#include <nfc/ndef/text_rec.h>

#include "laird_connectivity_nfc.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define NFC_HEADER			16
#define REC_OVERHEAD		5
#define NDEF_MSG_BUF_SIZE	(NFC_HEADER + (MAX_REC_PAYLOAD * MAX_REC_COUNT) + (REC_OVERHEAD * MAX_REC_COUNT))

/******************************************************************************/
/* Global Data Definitions                                                    */
/******************************************************************************/

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static const uint8_t en_code[] = "en";
#if (defined(CONFIG_LCZ_NFC_REC1_STR) && defined(CONFIG_LCZ_NFC_REC2_STR))
static const uint8_t p1_str[] = CONFIG_LCZ_NFC_REC1_STR;
static const uint8_t p2_str[] = CONFIG_LCZ_NFC_REC2_STR;
#else
#error "Two NFC record strings must be defined."
#endif

static uint8_t ndef_msg_buf[NDEF_MSG_BUF_SIZE];

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static void nfc_callback(void *context, enum nfc_t2t_event event,
			 const uint8_t *data, size_t data_length);
static int device_msg_encode(uint8_t *buffer, u32_t *len);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
int laird_connectivity_nfc_init()
{
	uint32_t ndef_len = NDEF_MSG_BUF_SIZE;
	int err = 0;

	/* initialize nfc for t2t operation */
	err = nfc_t2t_setup(nfc_callback, NULL);
	if (err < 0) {
		LOG_ERR("Failed to setup NFC T2T library. Error = %d", err);
	}

	/* generate the text records */
	err = device_msg_encode(ndef_msg_buf, &ndef_len);
	if (err < 0) {
		LOG_ERR("Failed to encode message. Error = %d", err);
	}

	err = nfc_t2t_payload_set(ndef_msg_buf, ndef_len);
	if (err < 0) {
		LOG_ERR("Failed to set payload. Error = %d", err);
	}

	err = nfc_t2t_emulation_start();
	if (err < 0) {
		LOG_ERR("Failed to start tag emulation. Error = %d", err);
	}

	return (err);
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static void nfc_callback(void *context,
			 enum nfc_t2t_event event,
			 const uint8_t *data,
			 size_t data_length)
{
	ARG_UNUSED(context);
	ARG_UNUSED(data);
	ARG_UNUSED(data_length);

	switch (event) {
	case NFC_T2T_EVENT_FIELD_ON:
		LOG_DBG("NFC field on");
		break;
	case NFC_T2T_EVENT_FIELD_OFF:
		LOG_DBG("NFC field off");
		break;
	default:
		LOG_DBG("Unhandled event = %d", event);
		break;
	}
}

static int device_msg_encode(uint8_t *buffer, u32_t *len)
{
	int err;

	NFC_NDEF_MSG_DEF(nfc_text_msg, MAX_REC_COUNT);


	NFC_NDEF_TEXT_RECORD_DESC_DEF(payload_1_rec,
				UTF_8,
				en_code,
				sizeof(en_code),
				p1_str,
				strlen(p1_str));
	NFC_NDEF_TEXT_RECORD_DESC_DEF(payload_2_rec,
				UTF_8,
				en_code,
				sizeof(en_code),
				p2_str,
				strlen(p2_str));

	err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_text_msg),
			&NFC_NDEF_TEXT_RECORD_DESC(payload_1_rec));
	if (err < 0) {
		LOG_ERR("Failed to add payload_1 record. Error = %d", err);
	}

	err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_text_msg),
			&NFC_NDEF_TEXT_RECORD_DESC(payload_2_rec));
	if (err < 0) {
		LOG_ERR("Failed to add payload_2 record. Error = %d", err);
	}

	err = nfc_ndef_msg_encode(&NFC_NDEF_MSG(nfc_text_msg),
				      buffer,
				      len);
	if (err < 0) {
		LOG_ERR("Failed to encode message. Error = %d", err);
	}

	return (err);
}

