/*
 * Copyright (c) 2020 Laird Connectivity
 */

#ifndef PIEZO_THREAD_EXAMPLE_H
#define PIEZO_THREAD_EXAMPLE_H

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

void test_piezo(void);
void stop_piezo(void);
bool is_piezo_running(void);
void setup_piezo_thread(void);

#ifdef __cplusplus
}
#endif

#endif /* PIEZO_THREAD_EXAMPLE_H */
