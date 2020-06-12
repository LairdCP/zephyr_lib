/*
 * Copyright (c) 2020 Laird Connectivity
 */

#ifndef WORK_QUEUE_H
#define WORK_QUEUE_H

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sensor_info {
	struct k_delayed_work piezo_work;
	struct k_delayed_work vibe_work;
	bool piezo_running;
	bool vibe_running;
};

extern struct sensor_info Sensor_work_status;

void setup_work_queue(void);

#ifdef __cplusplus
}
#endif

#endif /* WORK_QUEUE_H */
