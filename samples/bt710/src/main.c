/*
 * Copyright (c) 2020 Laird Connectivity
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <logging/log.h>
#include "version.h"
#include "workQueue.h"
#include "piezoThreadExample.h"
#include "vibeThreadExample.h"

void main(void)
{
	printk("Laird Connectivity %s\n", CONFIG_BOARD);
	setup_work_queue();
	setup_piezo_thread();
	setup_vibe_thread();

	/* The shell runs a loop so don't loop anything here, do it in a thread */
}
