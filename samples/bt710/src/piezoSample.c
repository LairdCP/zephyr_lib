/*
 * Copyright (c) 2020 Laird Connectivity
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <logging/log.h>
#include <device.h>
#include "piezo.h"
#include "piezoSample.h"
#include "workQueue.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(piezoSample, LOG_LEVEL);

#define BEEP_PERIOD 1500
#define BEEP_DUTY_CYCLE (1500 / 2U)
#define BEEP_DURATION K_MSEC(500)
void sample_beep(void)
{
	piezo_on(BEEP_PERIOD, BEEP_DUTY_CYCLE);
	Sensor_work_status.piezo_running = true;
	k_delayed_work_submit(&Sensor_work_status.piezo_work, BEEP_DURATION);
}
