/*
 * Copyright (c) 2020 Laird Connectivity
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>
#include <logging/log.h>
#include <stdlib.h>
#include "version.h"
#include "piezoSample.h"
#include "piezoThreadExample.h"
#include "vibeSample.h"
#include "vibeThreadExample.h"
#include "LedPwm.h"

LOG_MODULE_REGISTER(DebugShell);

static int beep_the_piezo(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	shell_print(shell, "beep piezo");
	sample_beep();
	return 0;
}

static int test_the_piezo(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	if (is_piezo_running() == true) {
		shell_print(shell, "piezo already running");
	} else {
		shell_print(shell, "starting piezo test");
		test_piezo();
	}
	return 0;
}

static int stop_the_piezo(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	if (is_piezo_running() == true) {
		shell_print(shell, "stopping the piezo test");
		stop_piezo();
	} else {
		shell_print(shell, "Test not running");
	}
	return 0;
}

static int vibrate(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	shell_print(shell, "vibrate");
	sample_vibrate();
	return 0;
}

static int test_vibrate(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	if (is_vibe_running() == true) {
		shell_print(shell, "vibe already running");
	} else {
		shell_print(shell, "starting vibe test");
		test_vibe();
	}
	return 0;
}

static int stop_vibrate(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	if (is_vibe_running() == true) {
		shell_print(shell, "stopping the vibe test");
		stop_vibe();
	} else {
		shell_print(shell, "Test not running");
	}
	return 0;
}

static int turn_on_led(const struct shell *shell, size_t argc, char **argv)
{
	if(argc < 5) {
		shell_print(shell, "usage: led on <led_index> <period> <duty_cycle>");
	}
	LedPwm_on(strtoul(argv[2], 10), strtoul(argv[3], 10), strtoul(argv[4], 10));
}

static int turn_off_led(const struct shell *shell, size_t argc, char **argv)
{
	if(argc < 3) {
		shell_print(shell, "usage: led off <led_index>");
	}
	LedPwm_off(strtoul(argv[2], 10));
}

static int turn_on_piezo(const struct shell *shell, size_t argc, char **argv)
{
	if(argc < 4) {
		shell_print(shell, "usage: piezo on <period> <duty_cycle>");
	}
	piezo_on(strtoul(argv[2], 10), strtoul(argv[3], 10));
	return 0;
}

static int turn_off_piezo(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	piezo_off();
	return 0;
}

static int turn_on_vibe(const struct shell *shell, size_t argc, char **argv)
{
	if(argc < 4) {
		shell_print(shell, "usage: vibe on <period> <duty_cycle>");
	}
	vibe_on(strtoul(argv[2], 10), strtoul(argv[3], 10));
	return 0;
}

static int turn_off_vibe(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	stop_vibe();
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_play, SHELL_CMD(beep, NULL, "beep piezo", beep_the_piezo),
	SHELL_CMD(testbeeps, NULL, "test piezo", test_the_piezo),
	SHELL_CMD(stopbeeps, NULL, "stop the piezo", stop_the_piezo),
	SHELL_CMD(vibe, NULL, "vibrate", vibrate),
	SHELL_CMD(testvibe, NULL, "test vibe", test_vibrate),
	SHELL_CMD(stopvibe, NULL, "stop the vibe", stop_vibrate),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(play, &sub_play, "play", NULL);
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_led, SHELL_CMD(on, NULL, "turn on LED at period and duty cycle", turn_on_led),
	SHELL_CMD(off, NULL, "turn off LED", turn_off_led),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(led, &sub_led, "led", NULL);
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_piezo, SHELL_CMD(on, NULL, "turn on piezo at period and duty cycle", turn_on_piezo),
	SHELL_CMD(off, NULL, "turn off piezo", turn_off_piezo),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(piezo, &sub_piezo, "piezo", NULL);
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_vibe, SHELL_CMD(on, NULL, "turn on vibe at period and duty cycle", turn_on_vibe),
	SHELL_CMD(off, NULL, "turn off vibe", turn_off_vibe),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(vibe, &sub_vibe, "vibe", NULL);
