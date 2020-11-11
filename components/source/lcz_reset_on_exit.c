/**
 * @file lcz_reset_on_exit.c
 * @brief Override weak implementation of _exit
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_reset_on_exit, LOG_LEVEL_ERR);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <power/reboot.h>
#include <logging/log_ctrl.h>

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
#ifdef CONFIG_LCZ_RESET_ON_EXIT
void _exit(int status)
{
	LOG_ERR("exit");
	LOG_PANIC();
	sys_reboot(SYS_REBOOT_COLD);
}
#endif
