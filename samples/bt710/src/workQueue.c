/*
 * Copyright (c) 2020 Laird Connectivity
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <logging/log.h>
#include "workQueue.h"
#include "piezo.h"
#include "vibe.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(workQueue, LOG_LEVEL);
struct sensor_info Sensor_work_status;

static void sample_stop_piezo(struct k_work *item)
{
	piezo_off();
}

static void sample_stop_vibe(struct k_work *item)
{
	vibe_off();
}

void setup_work_queue(void)
{
	k_delayed_work_init(&Sensor_work_status.piezo_work, sample_stop_piezo);
	k_delayed_work_init(&Sensor_work_status.vibe_work, sample_stop_vibe);
}
