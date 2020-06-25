/*
 * Copyright (c) 2020 Laird Connectivity
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <logging/log.h>
#include "piezo.h"
#include "WorkQueue.h"
#include "piezoThreadExample.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(piezoThreadExample, LOG_LEVEL);
#define PIEZO_PRIORITY K_PRIO_PREEMPT(5)
#define PIEZO_STACK_LENGTH 512
static K_THREAD_STACK_DEFINE(Piezo_stack_area, PIEZO_STACK_LENGTH);
static struct k_thread Piezo_thread_data;

#define PERIOD_MIN 50
#define PERIOD_MAX 3900
#define PERIOD_INIT 1500
#define BEEP_DURATION K_MSEC(60)
static uint32_t Period = PERIOD_INIT;
typedef enum { Test_Piezo, Beep_Piezo } set_piezo_state;
static struct k_sem Piezo_Wait_Sem;
static struct k_sem Piezo_Beep_Sem;
static set_piezo_state State;
static int Sema_State;
static bool Piezo_Running;

static void wait_for_input(void *unused1, void *unused2, void *unused3)
{
	Piezo_Running = false;
	uint8_t dir = 0U;
	k_sem_init(&Piezo_Wait_Sem, 0, 1);
	k_sem_init(&Piezo_Beep_Sem, 0, 1);
	while (true) {
		k_sem_take(&Piezo_Wait_Sem, K_FOREVER);
		switch (State) {
		case Test_Piezo:
			while (1) {
				Piezo_Running = true;
				if (piezo_on(Period, Period / 2) == false) {
					return;
				}
				k_sleep(BEEP_DURATION);
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
				Sema_State = k_sem_take(&Piezo_Beep_Sem,
							K_MSEC(1000));
				if (Sema_State == 0) {
					piezo_off();
					Piezo_Running = false;
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

void test_piezo(void)
{
	State = Test_Piezo;
	k_sem_give(&Piezo_Wait_Sem);
}

void stop_piezo(void)
{
	k_sem_give(&Piezo_Beep_Sem);
}

bool is_piezo_running(void)
{
	return Piezo_Running;
}

void setup_piezo_thread(void)
{
	k_thread_create(&Piezo_thread_data, Piezo_stack_area,
			K_THREAD_STACK_SIZEOF(Piezo_stack_area), wait_for_input,
			NULL, NULL, NULL, PIEZO_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&Piezo_thread_data, "piezo");
}
