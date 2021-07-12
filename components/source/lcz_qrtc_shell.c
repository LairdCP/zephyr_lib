/**
 * @file lcz_qrtc_shell.c
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <zephyr.h>
#include <shell/shell.h>
#include <init.h>
#include <stdio.h>
#include <stdlib.h>

#include "lcz_qrtc.h"

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static int shell_qrtc_set_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int rc = 0;

	if (argc != 2 || argv[1] == NULL) {
		shell_error(shell, "Invalid parameter");
		rc = -EINVAL;
	} else {
		uint32_t new_epoch = strtol(argv[1], NULL, 0);
		shell_print(shell, "%u", lcz_qrtc_set_epoch(new_epoch));
	}

	return rc;
}

static int shell_qrtc_get_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int rc = 0;

	shell_print(shell, "%u", lcz_qrtc_get_epoch());

	return rc;
}

static int shell_qrtc_isset_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int rc = 0;

	shell_print(shell, "%d", lcz_qrtc_epoch_was_set());

	return rc;
}

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
SHELL_STATIC_SUBCMD_SET_CREATE(
	qrtc_cmds, 
	SHELL_CMD(set, NULL, "QRTC set epoch", shell_qrtc_set_cmd),
	SHELL_CMD(get, NULL, "QRTC get epoch", shell_qrtc_get_cmd),
	SHELL_CMD(isset, NULL, "QRTC was set", shell_qrtc_isset_cmd),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(qrtc, &qrtc_cmds, "QRTC commands", NULL);
