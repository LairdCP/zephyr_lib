/*
 * Copyright (c) 2020 Laird Connectivity
 */

#ifndef PIEZO_WORK_QUEUE_H
#define PIEZO_WORK_QUEUE_H

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

void sample_beep(void);
void setup_piezo_work_queue(void);

#ifdef __cplusplus
}
#endif

#endif /* PIEZO_WORK_QUEUE_H */
