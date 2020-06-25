/*
 * Copyright (c) 2020 Laird Connectivity
 */

#ifndef PIEZO_H
#define PIEZO_H

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

bool piezo_on(uint32_t period, uint32_t pulseWidth);
bool piezo_off(void);
void piezo_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PIEZO_H */
