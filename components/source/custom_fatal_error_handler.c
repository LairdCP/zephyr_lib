/**
 * @file custom_fatal_error_handler.c
 * @brief Override the weak implementation.
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <kernel.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <kernel_structs.h>
#include <sys/printk.h>

#include "laird_power.h"

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void z_SysFatalErrorHandler(unsigned int reason, const z_arch_esf_t *pEsf)
{
	ARG_UNUSED(pEsf);

#if !defined(CONFIG_SIMPLE_FATAL_ERROR_HANDLER)
#ifdef CONFIG_STACK_SENTINEL
	if (reason == K_ERR_STACK_CHK_FAIL) {
		goto hang_system;
	}
#endif
	if (reason == K_ERR_KERNEL_PANIC) {
		goto hang_system;
	}
	if (k_is_in_isr() || z_is_thread_essential()) {
		printk("Fatal fault in %s! Spinning...\n",
		       k_is_in_isr() ? "ISR" : "essential thread");
		goto hang_system;
	}
	printk("Fatal fault in thread %p! Aborting.\n", _current);
	k_thread_abort(_current);
	return;

hang_system:
#else
	ARG_UNUSED(reason);
#endif

#ifdef CONFIG_REBOOT
	power_reboot_module(REBOOT_TYPE_NORMAL);
#endif

	for (;;) {
		k_cpu_idle();
	}
	CODE_UNREACHABLE;
}
