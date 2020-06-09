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
#include "piezoWorkQueue.h"
#include "piezoThreadExample.h"

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

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_piezo, SHELL_CMD(beep, NULL, "beep piezo", beep_the_piezo),
	SHELL_CMD(test, NULL, "test piezo", test_the_piezo),
	SHELL_CMD(stop, NULL, "stop the piezo", stop_the_piezo),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(piezo, &sub_piezo, "piezo", NULL);