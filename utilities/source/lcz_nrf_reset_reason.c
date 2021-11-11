/**
 * @file lcz_nrf_reset_reason.c
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <nrfx.h>
#include <hal/nrf_power.h>

#if !NRF_POWER_HAS_RESETREAS
#include <hal/nrf_reset.h>
#endif

#include "laird_utility_macros.h"
#include "lcz_nrf_reset_reason.h"

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
uint32_t lcz_nrf_reset_reason_get_and_clear_register(void)
{
	uint32_t reason;

#if NRF_POWER_HAS_RESETREAS
	reason = nrf_power_resetreas_get(NRF_POWER);
#else
	reason = nrf_reset_resetreas_get(NRF_RESET);
#endif

#if NRF_POWER_HAS_RESETREAS
	nrf_power_resetreas_clear(NRF_POWER, reason);
#else
	nrf_reset_resetreas_clear(NRF_RESET, reason);
#endif

	return reason;
}

const char *lcz_nrf_reset_reason_get_string(uint32_t reg)
{
	if (reg == 0) {
		return "POWER_UP";
	}

	/* Priority affects the result because multiple bits can be set. */
#if !NRF_POWER_HAS_RESETREAS
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, RESETPIN, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, DOG0, Msk);
#if defined(NRF52832_XXAA) || defined(NRF52833_XXAA) || defined(NRF52840_XXAA)
	IF_MASK_SET_RETURN_STRING(reg, NRFX_RESET_REASON, DOG0, MASK);
#endif
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, CTRLAP, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, SREQ, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, LOCKUP, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, OFF, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, LPCOMP, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, DIF, Msk);
#if NRF_RESET_HAS_NETWORK
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, LSREQ, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, LLOCKUP, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, LDOG, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, MFORCEOFF, Msk);
#endif
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, NFC, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, DOG1, Msk);
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, VBUS, Msk);
#if NRF_RESET_HAS_NETWORK
	IF_MASK_SET_RETURN_STRING(reg, RESET_RESETREAS, LCTRLAP, Msk);
#endif
#else
	IF_MASK_SET_RETURN_STRING(reg, POWER_RESETREAS, RESETPIN, Msk);
	IF_MASK_SET_RETURN_STRING(reg, POWER_RESETREAS, DOG, Msk);
	IF_MASK_SET_RETURN_STRING(reg, POWER_RESETREAS, SREQ, Msk);
	IF_MASK_SET_RETURN_STRING(reg, POWER_RESETREAS, LOCKUP, Msk);
	IF_MASK_SET_RETURN_STRING(reg, POWER_RESETREAS, OFF, Msk);
#if defined(POWER_RESETREAS_LPCOMP_Msk)
	IF_MASK_SET_RETURN_STRING(reg, POWER_RESETREAS, LPCOMP, Msk);
#endif
	IF_MASK_SET_RETURN_STRING(reg, POWER_RESETREAS, DIF, Msk);
#if defined(POWER_RESETREAS_NFC_Msk)
	IF_MASK_SET_RETURN_STRING(reg, POWER_RESETREAS, NFC, Msk);
#endif
#if defined(POWER_RESETREAS_VBUS_Msk)
	IF_MASK_SET_RETURN_STRING(reg, POWER_RESETREAS, VBUS, Msk);
#endif
#endif
	return "UNKNOWN";
}
