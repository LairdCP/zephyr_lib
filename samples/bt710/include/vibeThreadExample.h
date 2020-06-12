/*
 * Copyright (c) 2020 Laird Connectivity
 */

#ifndef VIBE_THREAD_EXAMPLE_H
#define VIBE_THREAD_EXAMPLE_H

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

void test_vibe(void);
void stop_vibe(void);
bool is_vibe_running(void);
void setup_vibe_thread(void);

#ifdef __cplusplus
}
#endif

#endif /* VIBE_THREAD_EXAMPLE_H */
