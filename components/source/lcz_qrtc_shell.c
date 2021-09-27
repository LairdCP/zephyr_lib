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
#if defined(CONFIG_LCZ_QRTC_SHELL_ENABLE_FORMATTED_OUTPUT)
#include <time.h>
#endif

#include "lcz_qrtc.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
/* QRTC times are printed in HH:MM:SS format (extra character for NULL) */
#define QRTC_TIME_STRING_SIZE 9
#define QRTC_TIME_STRING "%.2d:%.2d:%.2d"

/* QRTC dates are printed in DD/MM/YYYY format (extra character for NULL) */
#define QRTC_DATE_STRING_SIZE 11
#define QRTC_DATE_STRING "%.2d/%.2d/%.4d"

/* QRTC datetimes are printed in HH:MM:SS on DD/MM/YYYY format (extra character
 * for NULL)
 */
#define QRTC_TIMEDATE_STRING_SIZE 23
#define QRTC_TIMEDATE_STRING "%.2d:%.2d:%.2d on %.2d/%.2d/%.4d"

/* Additions to output current month/year from tm struct values */
#define QRTC_MONTH_OUTPUT_STARTING_MONTH 1
#define QRTC_YEAR_OUTPUT_STARTING_YEAR 1900

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/
#if defined(CONFIG_LCZ_QRTC_SHELL_ENABLE_WRITE)
static int shell_qrtc_set_cmd(const struct shell *shell, size_t argc,
			      char **argv)
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
#endif

static int shell_qrtc_get_cmd(const struct shell *shell, size_t argc,
			      char **argv)
{
	shell_print(shell, "%u", lcz_qrtc_get_epoch());

	return 0;
}

#if defined(CONFIG_LCZ_QRTC_SHELL_ENABLE_FORMATTED_OUTPUT)
static int shell_qrtc_gettime_cmd(const struct shell *shell, size_t argc,
				  char **argv)
{
	struct tm *tm;
        uint8_t time_string[QRTC_TIME_STRING_SIZE];
	time_t time = (time_t)lcz_qrtc_get_epoch();

	tm = gmtime(&time);
	snprintf(time_string, QRTC_TIME_STRING_SIZE, QRTC_TIME_STRING,
		 tm->tm_hour, tm->tm_min, tm->tm_sec);

	shell_print(shell, "%s", time_string);

	return 0;
}

static int shell_qrtc_getdate_cmd(const struct shell *shell, size_t argc,
				  char **argv)
{
	struct tm *tm;
        uint8_t time_string[QRTC_DATE_STRING_SIZE];
	time_t time = (time_t)lcz_qrtc_get_epoch();

	tm = gmtime(&time);
	snprintf(time_string, QRTC_DATE_STRING_SIZE, QRTC_DATE_STRING,
		 tm->tm_mday, tm->tm_mon + QRTC_MONTH_OUTPUT_STARTING_MONTH,
		 tm->tm_year + QRTC_YEAR_OUTPUT_STARTING_YEAR);

	shell_print(shell, "%s", time_string);

	return 0;
}

static int shell_qrtc_gettimedate_cmd(const struct shell *shell, size_t argc,
				      char **argv)
{
	struct tm *tm;
        uint8_t time_string[QRTC_TIMEDATE_STRING_SIZE];
	time_t time = (time_t)lcz_qrtc_get_epoch();

	tm = gmtime(&time);
	snprintf(time_string, QRTC_TIMEDATE_STRING_SIZE, QRTC_TIMEDATE_STRING,
		 tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_mday,
		 tm->tm_mon + QRTC_MONTH_OUTPUT_STARTING_MONTH,
		 tm->tm_year + QRTC_YEAR_OUTPUT_STARTING_YEAR);

	shell_print(shell, "%s", time_string);

	return 0;
}
#endif

static int shell_qrtc_isset_cmd(const struct shell *shell, size_t argc,
				char **argv)
{
	shell_print(shell, "%d", lcz_qrtc_epoch_was_set());

	return 0;
}

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
SHELL_STATIC_SUBCMD_SET_CREATE(qrtc_cmds,
#if defined(CONFIG_LCZ_QRTC_SHELL_ENABLE_WRITE)
			       SHELL_CMD(set, NULL, "QRTC set epoch",
					 shell_qrtc_set_cmd),
#endif
			       SHELL_CMD(get, NULL, "QRTC get epoch",
					 shell_qrtc_get_cmd),
#if defined(CONFIG_LCZ_QRTC_SHELL_ENABLE_FORMATTED_OUTPUT)
			       SHELL_CMD(gettime, NULL,
					 "QRTC get formatted time",
					 shell_qrtc_gettime_cmd),
			       SHELL_CMD(getdate, NULL,
					 "QRTC get formatted date",
					 shell_qrtc_getdate_cmd),
			       SHELL_CMD(gettimedate, NULL,
					 "QRTC get formatted time and date",
					 shell_qrtc_gettimedate_cmd),
#endif
			       SHELL_CMD(isset, NULL, "QRTC was set",
					 shell_qrtc_isset_cmd),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(qrtc, &qrtc_cmds, "QRTC commands", NULL);
