/*
 * Copyright (c) 2020 Laird Connectivity
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include "piezo.h"
#include <logging/log.h>
#include "piezoWorkQueue.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(PiezoWorkQueue, LOG_LEVEL);
#define PIEZO_STACK_SIZE 512
#define PIEZO_PRIORITY K_PRIO_PREEMPT(4)
static K_THREAD_STACK_DEFINE(Piezo_stack_area, PIEZO_STACK_SIZE);
static struct k_work_q Piezo_work_q;
static struct k_delayed_work Piezo_work;

#define BEEP_PERIOD 1500
#define BEEP_DUTY_CYCLE (1500 / 2U)
#define BEEP_DURATION K_MSEC(500)
void sample_beep(void)
{
	piezo_on(BEEP_PERIOD, BEEP_DUTY_CYCLE);
	k_delayed_work_submit(&Piezo_work, BEEP_DURATION);
}

void piezo_stop_work_queue(struct k_work *item)
{
	piezo_off();
}

void setup_piezo_work_queue(void)
{
	k_work_q_start(&Piezo_work_q, Piezo_stack_area,
		       K_THREAD_STACK_SIZEOF(Piezo_stack_area), PIEZO_PRIORITY);
	k_delayed_work_init(&Piezo_work, piezo_stop_work_queue);
}
