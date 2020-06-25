/*
 * Copyright (c) 2020 Laird Connectivity
 */

#ifndef VIBE_H
#define VIBE_H

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

bool vibe_on(uint32_t period, uint32_t pulseWidth);
bool vibe_off(void);
void vibe_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* VIBE_H */
