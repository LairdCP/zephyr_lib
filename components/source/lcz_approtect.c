/**
 * @file lcz_approtect.c
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <init.h>
#include <zephyr.h>
#include <stdlib.h>
#include <power/reboot.h>

#include "lcz_approtect.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#if !defined(NRF52_SERIES)
#error "lcz_approtect currently only supports nRF52 silicon"
#endif

#if defined(ENABLE_TRACE)
#warning "TRACE has been enabled as well as APPROTECT - is this intended?"
#endif

#if defined(CONFIG_LCZ_APPROTECT_STARTUP)
#if !defined(CONFIG_LCZ_APPROTECT_STARTUP_READBACK_PROTECTION) &&              \
    !defined(CONFIG_LCZ_APPROTECT_STARTUP_CPU_DEBUG_PROTECTION)
#error LCZ_APPROTECT_STARTUP is defined but is missing required                \
       LCZ_APPROTECT_STARTUP_READBACK_PROTECTION and/or                        \
       LCZ_APPROTECT_STARTUP_CPU_DEBUG_PROTECTION
#endif
#endif

/* Adapted from nrfx/mdk/system_nrf52_approtect.h */
#if NRF52_ERRATA_249_PRESENT
#if !defined(ENABLE_APPROTECT)
#warning "To enforce APPROTECT, ENABLE_APPROTECT should be enabled"
#endif
#endif

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
#define READBACK_PROTECTION_ENABLED_VALUE 0xffffff00
#define CPU_DEBUG_PROTECTION_ENABLED_VALUE 0xffff0000

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
#if defined(CONFIG_LCZ_APPROTECT_STARTUP)
static int approtect_sys_init(const struct device *device);
#endif

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
#if defined(CONFIG_LCZ_APPROTECT_STARTUP)
SYS_INIT(approtect_sys_init, PRE_KERNEL_1, 0);
#endif

bool lcz_enable_readback_protection(bool restart)
{
	uint32_t readback_protection = READBACK_PROTECTION_ENABLED_VALUE;

	if (NRF_UICR->APPROTECT != readback_protection) {
		/* Readback protection is disabled, enable it and reboot
		 * module. Enable flash to be written
		 */
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy);

		/* Write new value to flash (UICR) */
		((uint32_t*)(&NRF_UICR->APPROTECT))[0] = readback_protection;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy);

		/* Disable writing */
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy);

		/* Reboot device for changes to take effect, if requested */
		if (restart == true) {
			sys_reboot(SYS_REBOOT_COLD);
		}
		return true;
	}

	return false;
}

bool lcz_enable_cpu_debug_protection(bool restart)
{
	uint32_t cpu_debug_protection = CPU_DEBUG_PROTECTION_ENABLED_VALUE;

	if (NRF_UICR->DEBUGCTRL != cpu_debug_protection) {
		/* CPU debug protection is disabled, enable it and reboot
		 * module. Enable flash to be written
		 */
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy);

		/* Write new value to flash (UICR) */
		((uint32_t*)(&NRF_UICR->DEBUGCTRL))[0] = cpu_debug_protection;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy);

		/* Disable writing */
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy);

		/* Reboot device for changes to take effect, if requested */
		if (restart == true) {
			sys_reboot(SYS_REBOOT_COLD);
		}
		return true;
	}

	return false;
}

bool lcz_enable_cpu_debug_readback_protection(bool restart)
{
	bool restart_needed = lcz_enable_readback_protection(false);
	restart_needed |= lcz_enable_cpu_debug_protection(false);

	if (restart == true && restart_needed == true) {
		/* Reboot device for changes to take effect */
		sys_reboot(SYS_REBOOT_COLD);
	}

	return restart_needed;
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
#if defined(CONFIG_LCZ_APPROTECT_STARTUP)
static int approtect_sys_init(const struct device *device)
{
	ARG_UNUSED(device);

	bool restart_needed = false;
#if defined(CONFIG_LCZ_APPROTECT_STARTUP_READBACK_PROTECTION)
	restart_needed |= lcz_enable_readback_protection(false);
#endif
#if defined(CONFIG_LCZ_APPROTECT_STARTUP_CPU_DEBUG_PROTECTION)
	restart_needed |= lcz_enable_cpu_debug_protection(false);
#endif

	if (restart_needed == true) {
		/* Reboot device for changes to take effect */
		sys_reboot(SYS_REBOOT_COLD);
	}

	return 0;
}
#endif
