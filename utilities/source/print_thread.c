/**
 * @file thread_util.c
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <kernel.h>
#include "print_thread.h"

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static void print_thread_cb(const struct k_thread *thread, void *user_data);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
void print_thread_list(void)
{
	u32_t thread_count = 0;
	k_thread_foreach(print_thread_cb, &thread_count);
	printk("Preemption is %s\r\n",
	       (CONFIG_PREEMPT_ENABLED) ? "Enabled" : "Disabled");
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static void print_thread_cb(const struct k_thread *thread, void *user_data)
{
	u32_t *pc = (u32_t *)user_data;
	*pc += 1;
	/* discard const qualifier */
	struct k_thread *tid = (struct k_thread *)thread;
	printk("%02u id: (0x%08x) priority: %3d name: '%s' ", *pc, (u32_t)tid,
	       k_thread_priority_get(tid), k_thread_name_get(tid));
#if 0 /* not in this zephyr version. */
	printk("state %s ", k_thread_state_str(tid));
#endif
	printk("\r\n");
}
