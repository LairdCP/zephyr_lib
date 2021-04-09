/**
 * @file lcz_param_file_shell.c
 * @brief Test/Examples for parameter file module.
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <shell/shell.h>
#include <stdio.h>
#include <stdlib.h>

#include "file_system_utilities.h"
#include "lcz_param_file.h"

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static int lpf_parse_cmd(const struct shell *shell, size_t argc, char **argv);
static int lpf_gen_cmd(const struct shell *shell, size_t argc, char **argv);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_lpf,
	SHELL_CMD(
		parse, NULL,
		"parse a paramater file <full path> <optional: verbose if present>",
		lpf_parse_cmd),
	SHELL_CMD(gen, NULL, "generate a parameter file", lpf_gen_cmd),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(params, &sub_lpf, "Parameters", NULL);

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
static int lpf_parse_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int r;
	size_t fsize;
	char *fstr = NULL;
	param_kvp_t *kvp = NULL;

	if ((argc >= 2) && (argc <= 3) && (argv[1] != NULL)) {
		r = lcz_param_file_parse_from_file(argv[1], &fsize, &fstr,
						   &kvp);
		shell_print(shell, "pairs: %d fsize: %d", r, fsize);
		if (r > 0) {
			/* For simplicity, print if second argument is present. */
			if (argv[2] != NULL) {
				size_t i = 0;
				for (i = 0; i < r; i++) {
					shell_hexdump(shell,
						      (uint8_t *)&kvp[i].id, 2);
					shell_hexdump(shell, kvp[i].keystr,
						      kvp[i].length);
				}
			}
			k_free(kvp);
			k_free(fstr);
		}
	} else {
		shell_error(shell, "Unexpected parameters");
		return -EINVAL;
	}
	return 0;
}

static int lpf_gen_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int r;
	char *fstr = NULL;

	const uint8_t D0 = 1;
	const uint16_t D1 = 2;
	const uint32_t D2 = 0xF0000000;
	const int32_t I3 = -600000;
	const char STR[] = "This is an example";

	do {
		r = lcz_param_file_generate_file(0, PARAM_BIN, &D0, sizeof(D0),
						 &fstr);
		if (r < 0) {
			break;
		}

		r = lcz_param_file_generate_file(1, PARAM_BIN, &D1, sizeof(D1),
						 &fstr);
		if (r < 0) {
			break;
		}

		r = lcz_param_file_generate_file(2, PARAM_BIN, &D2, sizeof(D2),
						 &fstr);
		if (r < 0) {
			break;
		}

		r = lcz_param_file_generate_file(3, PARAM_BIN, &I3, sizeof(I3),
						 &fstr);
		if (r < 0) {
			break;
		}

		r = lcz_param_file_generate_file(4, PARAM_STR, STR, strlen(STR),
						 &fstr);
		if (r < 0) {
			break;
		}

	} while (0);

	if (r > 0) {
		shell_print(shell, "File contains %d pairs",
			    lcz_param_file_validate_file(fstr, strlen(fstr)));
	}

	shell_hexdump(shell, fstr, strlen(fstr));

	ssize_t write_status =
		fsu_write_abs(CONFIG_LCZ_PARAM_FILE_MOUNT_POINT "/gen.txt",
			      fstr, strlen(fstr));
	shell_print(shell, "Write status %d", write_status);

	k_free(fstr);
	return 0;
}
