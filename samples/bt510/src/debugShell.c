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

LOG_MODULE_REGISTER(DebugShell);

static int reset_the_board(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	shell_print(shell, "resetting");
	k_sleep(K_MSEC(500));
	NVIC_SystemReset();
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_play, SHELL_CMD(reset, NULL, "reset the board", reset_the_board),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(play, &sub_play, "play", NULL);