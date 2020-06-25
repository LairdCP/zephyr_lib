/*
 * Copyright (c) 2020 Laird Connectivity
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <logging/log.h>
#include "vibe.h"
#include "WorkQueue.h"
#include "vibeThreadExample.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(vibeThreadExample, LOG_LEVEL);
#define VIBE_PRIORITY K_PRIO_PREEMPT(6)
#define VIBE_STACK_LENGTH 512
static K_THREAD_STACK_DEFINE(Vibe_stack_area, VIBE_STACK_LENGTH);
static struct k_thread Vibe_thread_data;

#define PERIOD_MIN 50000
#define PERIOD_MAX 200000
#define PERIOD_INIT 100000
#define VIBE_DURATION K_MSEC(500)
static uint32_t Period = PERIOD_INIT;
typedef enum { Test_Vibe } set_vibe_state;
static struct k_sem Vibe_Wait_Sem;
static struct k_sem Vibe_Beep_Sem;
static set_vibe_state State;
static int Sema_State;
static bool Vibe_Running;

static void wait_for_input(void *unused1, void *unused2, void *unused3)
{
	Vibe_Running = false;
	uint8_t dir = 0U;
	k_sem_init(&Vibe_Wait_Sem, 0, 1);
	k_sem_init(&Vibe_Beep_Sem, 0, 1);
	while (true) {
		k_sem_take(&Vibe_Wait_Sem, K_FOREVER);
		switch (State) {
		case Test_Vibe:
			while (1) {
				Vibe_Running = true;
				if (vibe_on(Period, Period / 2) == false) {
					return;
				}
				k_sleep(VIBE_DURATION);
				if (dir) {
					if (Period < PERIOD_MIN) {
						dir = 0U;
					} else {
						Period -= PERIOD_MIN;
					}
				} else {
					Period += PERIOD_MIN;

					if (Period >= PERIOD_MAX) {
						dir = 1U;
						Period = PERIOD_MAX;
					}
				}
				Sema_State = k_sem_take(&Vibe_Beep_Sem,
							K_MSEC(1000));
				if (Sema_State == 0) {
					vibe_off();
					Vibe_Running = false;
					break;
				}
			}
			break;
		default:
			LOG_ERR("erroneous beep state: %d", State);
			break;
		}
	}
}

void test_vibe(void)
{
	State = Test_Vibe;
	k_sem_give(&Vibe_Wait_Sem);
}

void stop_vibe(void)
{
	k_sem_give(&Vibe_Beep_Sem);
}

bool is_vibe_running(void)
{
	return Vibe_Running;
}

void setup_vibe_thread(void)
{
	k_thread_create(&Vibe_thread_data, Vibe_stack_area,
			K_THREAD_STACK_SIZEOF(Vibe_stack_area), wait_for_input,
			NULL, NULL, NULL, VIBE_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&Vibe_thread_data, "vibe");
}
