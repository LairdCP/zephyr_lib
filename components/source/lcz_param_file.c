/**
 * @file lcz_param_file.c
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_param_file, CONFIG_LCZ_PARAM_FILE_LOG_LEVEL);

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <init.h>
#include <zephyr.h>
#include <fs/fs.h>
#include <sys/util.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "file_system_utilities.h"
#include "lcz_param_file.h"

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/
#define BREAK_ON_ERROR(x)                                                      \
	if (x < 0) {                                                           \
		break;                                                         \
	}

#define SIZE_OF_NUL 1
#define SIZE_OF_DELIMITER 1

#define DELIMITER_CHAR '='
#define EOL_CHAR '\n'
#define CR_CHAR '\r'

#define PARAMS_MAX_FILE_SIZE (CONFIG_LCZ_PARAM_FILE_MAX_FILE_LENGTH + 1)

#define PARAMS_MAX_ID_BYTES sizeof(param_id_t)
#define PARAMS_MAX_ID_LENGTH (PARAMS_MAX_ID_BYTES * 2)

#define PARAMS_PATH                                                            \
	CONFIG_LCZ_PARAM_FILE_MOUNT_POINT "/" CONFIG_LCZ_PARAM_FILE_PATH

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static bool params_ready;

BUILD_ASSERT(sizeof(PARAMS_PATH) <= CONFIG_FSU_MAX_PATH_SIZE,
	     "Params path too long");

/******************************************************************************/
/* Local Function Prototypes                                                  */
/******************************************************************************/
static int lcz_param_file_init(const struct device *device);

static int read_text(const char *fname, char **fstr, size_t fsize);
static int read_and_remove_cr(char *str, struct fs_file_t *fptr, size_t fsize,
			      size_t *length);
static int parse_file(const char *str, int pairs, param_kvp_t *kv);

static int append_parameter(param_id_t id, param_t type, const void *data,
			    size_t dsize, char *str);
static int append_id(char *str, size_t *length, param_id_t id);
static int append_value(char *str, size_t *length, param_t type,
			const void *data, size_t dsize);

static bool room_for_id(size_t current_length);
static bool room_for_value(size_t current_length, size_t dsize);

/******************************************************************************/
/* Global Function Definitions                                                */
/******************************************************************************/
/* Initialize after fs but before application. */
SYS_INIT(lcz_param_file_init, APPLICATION, CONFIG_LCZ_PARAM_FILE_INIT_PRIORITY);

ssize_t lcz_param_file_write(char *name, void *data, size_t size)
{
	if (params_ready) {
		return fsu_write(PARAMS_PATH, name, data, size);
	} else {
		return -EPERM;
	}
}

ssize_t lcz_param_file_read(char *name, void *data, size_t size)
{
	if (params_ready) {
		return fsu_read(PARAMS_PATH, name, data, size);
	} else {
		return -EPERM;
	}
}

int lcz_param_file_delete(char *name)
{
	if (params_ready) {
		return fsu_delete(PARAMS_PATH, name);
	} else {
		return -EPERM;
	}
}

int lcz_param_file_parse_from_file(const char *fname, size_t *fsize,
				   char **fstr, param_kvp_t **kv)
{
	int r = -EPERM;
	struct fs_dirent *entry = k_malloc(sizeof(struct fs_dirent));
	*fsize = 0;
	*fstr = NULL;
	*kv = NULL;
	do {
		if (entry == NULL) {
			r = -ENOMEM;
			LOG_ERR("Allocation failure");
			break;
		}

		r = fs_stat(fname, entry);
		if (r < 0) {
			LOG_ERR("%s not found", log_strdup(fname));
			break;
		}

		LOG_DBG("'%s' parameter file size bytes: %u ",
			log_strdup(fname), entry->size);
		if (entry->size == 0) {
			r = -EPERM;
			LOG_ERR("Unexpected file size");
			break;
		}

		r = read_text(fname, fstr, entry->size);
		BREAK_ON_ERROR(r);

		entry->size = (size_t)r;
		if (IS_ENABLED(CONFIG_LCZ_PARAM_FILE_LOG_VERBOSE)) {
			LOG_DBG("stripped size: %u ", entry->size);
		}
		r = lcz_param_file_validate_file(*fstr, entry->size);
		BREAK_ON_ERROR(r);

		/* allocate key-pointer-to-value pairs */
		*kv = k_calloc(r, sizeof(param_kvp_t));
		if (*kv == NULL) {
			r = -ENOMEM;
			break;
		}

		r = parse_file(*fstr, r, *kv);
		BREAK_ON_ERROR(r);

	} while (0);

	*fsize = entry->size;
	k_free(entry);
	if (r < 0) {
		k_free(*fstr);
		k_free(*kv);
	}

	return r;
}

int lcz_param_file_generate_file(param_id_t id, param_t type, const void *data,
				 size_t dsize, char **fstr)
{
	int r = 0;
	do {
		if (fstr == NULL) {
			r = -EPERM;
			break;
		}

		if (*fstr == NULL) {
			*fstr = k_calloc(PARAMS_MAX_FILE_SIZE, sizeof(char));
		}

		if (*fstr == NULL) {
			r = -ENOMEM;
			break;
		}

		r = append_parameter(id, type, data, dsize, *fstr);

	} while (0);

	return r;
}

int lcz_param_file_validate_file(const char *str, size_t length)
{
	int r = 0;
	int delimiters = 0;
	int newlines = 0;
	int distance = 0;
	size_t i;
	for (i = 0; i < length; i++) {
		if (str[i] == DELIMITER_CHAR) {
			delimiters += 1;
			if (distance > PARAMS_MAX_ID_LENGTH) {
				r = -EINVAL;
				LOG_ERR("Unexpected id size at %u", i);
				break;
			} else {
				if (IS_ENABLED(
					    CONFIG_LCZ_PARAM_FILE_LOG_VERBOSE)) {
					LOG_DBG("%u %d", i, distance);
				}
				distance = 0;
			}
		} else if (str[i] == EOL_CHAR) {
			newlines += 1;
			if (distance > CONFIG_LCZ_PARAM_FILE_MAX_VALUE_LENGTH) {
				r = -EINVAL;
				LOG_ERR("Invalid Size of %d at %u", distance,
					i);
				break;
			}
			if (IS_ENABLED(CONFIG_LCZ_PARAM_FILE_LOG_VERBOSE)) {
				LOG_DBG("%u %d", i, distance);
			}
			distance = 0;
		} else if (!isprint((int)str[i])) {
			r = -EINVAL;
			LOG_ERR("Non-printable char 0x%x at %u", str[i], i);
			break;
		} else {
			distance += 1;
		}
	}

	if (delimiters != newlines) {
		r = -EINVAL;
		LOG_ERR("%d delimeters != %d newlines", delimiters, newlines);
	} else {
		LOG_DBG("Found %d pairs", delimiters);
	}
	return (r < 0) ? r : delimiters;
}

/******************************************************************************/
/* Local Function Definitions                                                 */
/******************************************************************************/

/**
 * @retval negative on error, otherwise number of key-value pairs.
 */
static int parse_file(const char *str, int pairs, param_kvp_t *kv)
{
	int r = 0;
	char *start = (char *)str;
	char *end;
	char *delimiter;
	char *newline;
	int i;
	for (i = 0; i < pairs; i++) {
		delimiter = strchr(start, DELIMITER_CHAR);
		kv[i].id = strtoul(start, &end, 16);
		if (delimiter != end) {
			r = -1;
			LOG_ERR("Unexpected delimiter position");
			break;
		}
		kv[i].keystr = end + 1;
		newline = strchr(kv[i].keystr, EOL_CHAR);
		kv[i].length = newline - kv[i].keystr;
		if (kv[i].length < 1) {
			LOG_WRN("Unexpected parameter length (possible empty string)");
		}
		start = newline + 1;
	}

	return i;
}

/**
 * @brief Read the text file from the filesystem into a buffer in RAM.
 *
 * @retval negative on error, otherwise size of string with carriage returns
 * removed.
 */
static int read_text(const char *fname, char **fstr, size_t fsize)
{
	int r = 0;
	size_t length = 0;
	struct fs_file_t file;
	char *str = NULL;

	do {
		str = k_malloc(fsize);
		if (str == NULL) {
			r = -ENOMEM;
			LOG_ERR("Unable to allocate parameter string");
			break;
		}
		*fstr = str;

		r = fs_open(&file, fname, FS_O_READ);
		BREAK_ON_ERROR(r);

		r = read_and_remove_cr(str, &file, fsize, &length);

		/* The validator checks that the file ends with a '\n' (0x0A). */
		if (IS_ENABLED(CONFIG_LCZ_PARAM_FILE_LOG_VERBOSE) &&
		    length > 1) {
			LOG_DBG("0x%x 0x%x", str[length - 2], str[length - 1]);
		}

		r = fs_close(&file);
		BREAK_ON_ERROR(r);

	} while (0);

	return (r < 0) ? r : length;
}

/* Removing carriage returns makes parsing easier */
static int read_and_remove_cr(char *str, struct fs_file_t *fptr, size_t fsize,
			      size_t *length)
{
	int r = 0;
	size_t i;
	char c;
	for (i = 0; i < fsize; i++) {
		r = fs_read(fptr, &c, 1);
		if (r == 1) {
			if (c != CR_CHAR) {
				str[(*length)++] = c;
			}
		} else {
			r = -EINVAL;
			LOG_ERR("file read error");
			break;
		}
	}

	return r;
}

static int append_parameter(param_id_t id, param_t type, const void *data,
			    size_t dsize, char *str)
{
	int r = -EPERM;
	size_t length = strlen(str);
	size_t starting_length = length;

	do {
		r = append_id(str, &length, id);
		BREAK_ON_ERROR(r);

		r = append_value(str, &length, type, data, dsize);
		BREAK_ON_ERROR(r);

		r = length - starting_length;

		if (IS_ENABLED(CONFIG_LCZ_PARAM_FILE_LOG_VERBOSE)) {
			LOG_HEXDUMP_DBG(&str[starting_length], r,
					"append parameter");
		}

	} while (0);

	return r;
}

/* The bin2hex function returns a null terminated string.  The
 * size check for the delimiter character ensures room for the terminator.
 */
static bool room_for_id(size_t current_length)
{
	return ((current_length + PARAMS_MAX_ID_LENGTH + SIZE_OF_DELIMITER) <
		CONFIG_LCZ_PARAM_FILE_MAX_FILE_LENGTH);
}

static int append_id(char *str, size_t *length, param_id_t id)
{
	int r = -EPERM;
	int id_len;
	/* id is big endian hex */
	if (room_for_id(*length)) {
		if (IS_ENABLED(CONFIG_LCZ_PARAM_FILE_4_DIGIT_ID)) {
			id_len = snprintf(&str[*length], PARAMS_MAX_ID_LENGTH,
					  "%04x", id);
		} else {
			id_len = snprintf(&str[*length], PARAMS_MAX_ID_LENGTH,
					  "%x", id);
		}
		if (id_len <= 0) {
			r = -EINVAL;
			LOG_ERR("Id conversion error");
		} else {
			r = 0;
			*length += id_len;
			str[*length] = DELIMITER_CHAR;
			*length += SIZE_OF_DELIMITER;
		}
	} else {
		LOG_ERR("Parameter file string too small");
	}
	return r;
}

/*
 * The file is built as a string and must be null teminated until
 * it is written to the filesystem (or sent over a transport).
 */
static bool room_for_value(size_t current_length, size_t dsize)
{
	return ((dsize < CONFIG_LCZ_PARAM_FILE_MAX_VALUE_LENGTH) &&
		((current_length + dsize + SIZE_OF_DELIMITER + SIZE_OF_NUL) <
		 CONFIG_LCZ_PARAM_FILE_MAX_FILE_LENGTH));
}

static int append_value(char *str, size_t *length, param_t type,
			const void *data, size_t dsize)
{
	int r = -EPERM;
	size_t val_len = 0;
	if (room_for_value(*length, dsize)) {
		switch (type) {
		case PARAM_BIN:
			val_len =
				bin2hex(data, dsize, &str[*length],
					CONFIG_LCZ_PARAM_FILE_MAX_VALUE_LENGTH +
						SIZE_OF_NUL);
			if (val_len == 0) {
				r = -EINVAL;
				LOG_ERR("Value conversion error");
			} else {
				r = 0;
			}
			break;

		case PARAM_STR:
			r = 0;
			val_len = dsize;
			memcpy(&str[*length], data, dsize);
			break;

		default:
			LOG_ERR("Unknown parameter type");
			break;
		}

		if (r == 0) {
			*length += val_len;
			str[*length] = EOL_CHAR;
			*length += SIZE_OF_DELIMITER;
			str[*length] = '\0';
			/* Don't update length for terminator or it won't equal strlen */
		}
	} else {
		LOG_ERR("Parameter file string too small");
	}

	return r;
}

static int lcz_param_file_init(const struct device *device)
{
	ARG_UNUSED(device);
	int r = -EPERM;

	do {
		r = lcz_param_file_mount_fs();
		BREAK_ON_ERROR(r);

		if (strlen(CONFIG_LCZ_PARAM_FILE_PATH)) {
			r = fsu_mkdir(CONFIG_LCZ_PARAM_FILE_MOUNT_POINT,
				      CONFIG_LCZ_PARAM_FILE_PATH);
			BREAK_ON_ERROR(r);
		}

		params_ready = true;
		r = 0;
	} while (0);

	LOG_DBG("Init status: %d", r);
	return r;
}

/******************************************************************************/
/* If desired, override in application                                        */
/******************************************************************************/
__weak int lcz_param_file_mount_fs(void)
{
	if (strcmp(CONFIG_LCZ_PARAM_FILE_MOUNT_POINT, CONFIG_FSU_MOUNT_POINT) ==
	    0) {
		return fsu_lfs_mount();
	} else {
		return -EPERM;
	}
}
