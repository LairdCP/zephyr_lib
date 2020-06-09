/*
 * Copyright (c) 2020 Laird Connectivity
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <logging/log.h>
#include "version.h"
#include "piezoWorkQueue.h"
#include "piezoThreadExample.h"

void main(void)
{
	printk("Laird Connectivity %s\n", CONFIG_BOARD);
	setup_piezo_work_queue();
	setup_piezo_thread();

	/* The shell runs a loop so don't loop anything here, do it in a thread */
}
