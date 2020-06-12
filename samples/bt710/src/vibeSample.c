/*
 * Copyright (c) 2020 Laird Connectivity
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <logging/log.h>
#include "vibe.h"
#include "vibeSample.h"
#include "workQueue.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(vibeSample, LOG_LEVEL);

#define VIBE_PERIOD 100000
#define VIBE_DUTY_CYCLE (VIBE_PERIOD / 2U)
#define VIBE_DURATION K_MSEC(500)
void sample_vibrate(void)
{
	vibe_on(VIBE_PERIOD, VIBE_DUTY_CYCLE);
	Sensor_work_status.vibe_running = true;
	k_delayed_work_submit(&Sensor_work_status.vibe_work, VIBE_DURATION);
}
