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
#include <string.h>

#include "string_util.h"
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
	uint32_t thread_count = 0;
	k_thread_foreach(print_thread_cb, &thread_count);
	printk("Preemption is %s\r\n",
	       (CONFIG_PREEMPT_ENABLED) ? "Enabled" : "Disabled");
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static void print_thread_cb(const struct k_thread *thread, void *user_data)
{
	uint32_t *pc = (uint32_t *)user_data;
	*pc += 1;
	/* discard const qualifier */
	struct k_thread *tid = (struct k_thread *)thread;
	size_t max_size = CONFIG_THREAD_MAX_NAME_LEN + 1;
	char thread_name[max_size];
	left_justify(thread_name, k_thread_name_get(tid), max_size, ' ');
	printk("%02u id: (0x%08x) prio: %3d '%s'", *pc, (uint32_t)tid,
	       k_thread_priority_get(tid), thread_name);

	size_t unused = 0;
	k_thread_stack_space_get(thread, &unused);
	printk(" stack size: %5u used: %5u unused: %5u", tid->stack_info.size,
	       (tid->stack_info.size - unused), unused);

	printk(" state: %s ", k_thread_state_str(tid));
	printk("\r\n");
}
