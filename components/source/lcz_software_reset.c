/**
 * @file lcz_software_reset.c
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_software_reset, CONFIG_LCZ_SOFTWARE_RESET_LOG_LEVEL);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <kernel.h>
#include <logging/log_ctrl.h>
#include <power/reboot.h>

#include "lcz_software_reset.h"

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void lcz_software_reset(uint32_t delay_ms)
{
	LOG_PANIC();
	LOG_WRN("Software Reset in %d milliseconds", delay_ms);
	k_sleep(K_MSEC(delay_ms));
	sys_reboot(SYS_REBOOT_COLD);
}
