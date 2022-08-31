/**
 * @file lcz_kvp.c
 * @brief
 *
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_kvp, CONFIG_LCZ_KVP_LOG_LEVEL);

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <init.h>
#include <zephyr.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#if defined(CONFIG_FSU_ENCRYPTED_FILES)
#include "encrypted_file_storage.h"
#endif
#include "file_system_utilities.h"
#include "lcz_kvp.h"

/**************************************************************************************************/
/* Local Constant, Macro and Type Definitions                                                     */
/**************************************************************************************************/
#ifdef CONFIG_LCZ_KVP_LOG_HEXDUMP
#define KVP_HEXDUMP LOG_HEXDUMP_DBG
#else
#define KVP_HEXDUMP(...)
#endif

#ifdef CONFIG_LCZ_KVP_LOG_VERBOSE
#define LOG_VRB LOG_DBG
#else
#define LOG_VRB(...)
#endif

typedef struct func_context {
	ssize_t (*get_size)(const char *abs_path);
	ssize_t (*read)(const char *abs_path, void *data, size_t size);
	ssize_t (*write)(const char *abs_path, void *data, size_t size);
	ssize_t (*append)(const char *abs_path, void *data, size_t size);
	const char *msg;
} func_context_t;

#define TERMINATOR '\0'
#define DELIMITER '='
#define COMMENT_CHAR '#'
#define CR_CHAR '\r'
#define EOL_CHAR '\n'

#define APPEND(k, l)                                                                               \
	memcpy(&str[length], (k), (l));                                                            \
	length += (l)

#define APPEND_CHAR(c) str[length++] = (c)

/**************************************************************************************************/
/* Local Data Definitions                                                                         */
/**************************************************************************************************/
static bool kvp_ready;

/**************************************************************************************************/
/* Local Function Prototypes                                                                      */
/**************************************************************************************************/
static func_context_t get_func_context(bool encrypted);

static int read_text(const func_context_t *ctx, const char *fname, char **fstr, size_t fsize);

static char *parse_kvp(const lcz_kvp_cfg_t *cfg, char *start, lcz_kvp_t *kvp);
static int parse_kvp_file(const lcz_kvp_cfg_t *cfg, const char *str, int pairs, lcz_kvp_t *kv);

static int append_kvp_line(const lcz_kvp_cfg_t *cfg, const lcz_kvp_t *kvp, char *str);

static bool valid_cfg(const lcz_kvp_cfg_t *cfg);
static bool valid_kvp(const lcz_kvp_cfg_t *cfg, const lcz_kvp_t *kvp);
static ssize_t space_avail(const lcz_kvp_cfg_t *cfg, const lcz_kvp_t *kvp, size_t start_len);

/**************************************************************************************************/
/* SYS INIT                                                                                       */
/**************************************************************************************************/
static int lcz_kvp_init(const struct device *device)
{
	ARG_UNUSED(device);
	int r = -EPERM;

	do {
		r = fsu_lfs_mount();
		if (r < 0) {
			break;
		}

		kvp_ready = true;
		r = 0;
	} while (0);

	LOG_DBG("Init status: %d", r);

	return 0;
}

/* Initialize after fs but before application modules that may use it */
SYS_INIT(lcz_kvp_init, APPLICATION, CONFIG_LCZ_KVP_INIT_PRIORITY);

/**************************************************************************************************/
/* Global Function Definitions                                                                    */
/**************************************************************************************************/
ssize_t lcz_kvp_write(bool encrypted, char *name, void *data, size_t size)
{
	char abs_path[FSU_MAX_ABS_PATH_SIZE];
	func_context_t ctx;
	int r;

	if (kvp_ready) {
		(void)fsu_build_full_name(abs_path, sizeof(abs_path), CONFIG_FSU_MOUNT_POINT, name);
		ctx = get_func_context(encrypted);
		if (ctx.write == NULL) {
			r = -ENOTSUP;
		} else {
			r = ctx.write(abs_path, data, size);
		}
	} else {
		r = -EPERM;
	}

	return r;
}

ssize_t lcz_kvp_read(bool encrypted, char *name, void *data, size_t size)
{
	char abs_path[FSU_MAX_ABS_PATH_SIZE];
	func_context_t ctx;
	int r;

	if (kvp_ready) {
		(void)fsu_build_full_name(abs_path, sizeof(abs_path), CONFIG_FSU_MOUNT_POINT, name);
		ctx = get_func_context(encrypted);
		if (ctx.read == NULL) {
			r = -ENOTSUP;
		} else {
			r = ctx.read(abs_path, data, size);
		}
	} else {
		r = -EPERM;
	}

	return r;
}

int lcz_kvp_delete(char *name)
{
	if (kvp_ready) {
		return fsu_delete(CONFIG_FSU_MOUNT_POINT, name);
	} else {
		return -EPERM;
	}
}

int lcz_kvp_parse_from_file(const lcz_kvp_cfg_t *cfg, const char *fname, size_t *fsize, char **fstr,
			    lcz_kvp_t **kv)
{
	int r = -EPERM;
	ssize_t file_size;
	func_context_t ctx;

	*fsize = 0;
	*fstr = NULL;
	*kv = NULL;

	if (!valid_cfg(cfg)) {
		LOG_ERR("Invalid kvp config");
		return -EINVAL;
	}

	ctx = get_func_context(cfg->encrypted);
	if (ctx.get_size == NULL) {
		return -ENOTSUP;
	}

	do {
		file_size = ctx.get_size(fname);
		if (file_size < 0) {
			LOG_ERR("Could not get size of %s param file %s: %d", ctx.msg, fname,
				file_size);
			r = *fsize;
			break;
		}

		LOG_DBG("'%s' %s kvp file size bytes: %u ", fname, ctx.msg, file_size);
		if (file_size == 0) {
			r = -ENOENT;
			LOG_ERR("%s param file %s is empty", ctx.msg, fname);
			break;
		}

		r = read_text(&ctx, fname, fstr, file_size);
		if (r < 0) {
			break;
		}
		file_size = r;

		LOG_VRB("stripped size: %u ", file_size);

		r = lcz_kvp_validate_file(cfg, *fstr, file_size);
		if (r < 0) {
			break;
		}

		/* allocate key-pointer-to-value pairs */
		*kv = k_calloc(r, sizeof(lcz_kvp_t));
		if (*kv == NULL) {
			r = -ENOMEM;
			break;
		}

		r = parse_kvp_file(cfg, *fstr, r, *kv);
		if (r < 0) {
			break;
		}

	} while (0);

	*fsize = file_size;
	if (r < 0) {
		/* Clear encrypted file data */
		if (*fstr != NULL) {
			memset(*fstr, 0, *fsize);
			k_free(*fstr);
		}
		k_free(*kv);
		*fstr = NULL;
		*kv = NULL;
	}

	return r;
}

int lcz_kvp_generate_file(const lcz_kvp_cfg_t *cfg, const lcz_kvp_t *kvp, char **fstr)
{
	int r = 0;

	do {
		if (!valid_cfg(cfg)) {
			LOG_ERR("Invalid kvp config");
			r = -EINVAL;
			break;
		}

		if (fstr == NULL) {
			LOG_ERR("Invalid kvp file ptr");
			r = -EPERM;
			break;
		}

		if (*fstr == NULL) {
			*fstr = k_calloc(cfg->max_file_size, sizeof(char));
		}

		if (*fstr == NULL) {
			LOG_ERR("Unable to allocate kvp file");
			r = -ENOMEM;
			break;
		}

		r = append_kvp_line(cfg, kvp, *fstr);

	} while (0);

	return r;
}

int lcz_kvp_validate_file(const lcz_kvp_cfg_t *cfg, const char *str, size_t size)
{
	int r = 0;
	size_t delimiters = 0;
	size_t lines = 0;
	size_t pairs = 0;
	bool comment = false;
	size_t distance = 0;
	size_t i;

	if (size > cfg->max_file_size) {
		LOG_ERR("KVP file too large %u > %u (max)", size, cfg->max_file_size);
		return -EINVAL;
	}

	for (i = 0; i < size; i++) {
		if (str[i] == DELIMITER) {
			delimiters += 1;
			if (distance <= 0 || distance > cfg->max_key_len) {
				r = -EINVAL;
				LOG_ERR("Invalid key length: %u line: %u", distance, lines);
				break;
			}
			distance = 0;
		} else if (str[i] == EOL_CHAR) {
			lines += 1;
			if (!comment) {
				pairs += 1;
				if (distance <= 0) {
					r = -EINVAL;
					LOG_ERR("Invalid value length: %u line: %u", distance,
						lines);
					break;
				}
			}
			comment = false;
			distance = 0;
		} else if (str[i] == COMMENT_CHAR) {
			if (!comment) {
				comment = true;
				if (distance != 0) {
					r = -EINVAL;
					LOG_ERR("Comment character must start line: %u pos: %u abs pos: %u",
						lines + 1, distance, i);
					break;
				}
			}
		} else if (!isprint((int)str[i])) {
			r = -EINVAL;
			LOG_ERR("Non-printable char 0x%x at line: %u pos: %u absolute pos: %u",
				str[i], lines + 1, distance, i);
			break;
		} else {
			distance += 1;
		}
	}

	KVP_HEXDUMP(str, size, "kvp str");

	if (delimiters != pairs) {
		r = -EINVAL;
	}
	LOG_DBG("Found %u pairs status: %d", pairs, r);

	return (r < 0) ? r : pairs;
}

/**************************************************************************************************/
/* Local Function Definitions                                                                     */
/**************************************************************************************************/
static char *parse_kvp(const lcz_kvp_cfg_t *cfg, char *start, lcz_kvp_t *kvp)
{
	char *next = NULL;
	char *delimiter;
	char *newline;

	delimiter = strchr(start, DELIMITER);
	newline = strchr(delimiter, EOL_CHAR);

	do {
		if (delimiter == NULL) {
			LOG_ERR("Delimiter not found");
			break;
		}

		/* Strings cannot contain eol or newlines */
		if (newline == NULL) {
			LOG_ERR("Newline not found");
			break;
		}

		kvp->key = start;
		kvp->key_len = delimiter - start;
		kvp->val = delimiter + 1;
		kvp->val_len = newline - kvp->val;

		/* If the value matches "", then this is any empty string */
		if (kvp->val_len == strlen(LCZ_KVP_EMPTY_VALUE_STR)) {
			if (memcmp(LCZ_KVP_EMPTY_VALUE_STR, kvp->val, kvp->val_len) == 0) {
				kvp->val_len = 0;
			}
		}

		if (kvp->key_len > cfg->max_key_len) {
			LOG_ERR("Invalid key length: %d", kvp->key_len);
			KVP_HEXDUMP(kvp->key, kvp->key_len, "Invalid key");
			break;
		}

		if (kvp->val_len > cfg->max_val_len) {
			LOG_ERR("Invalid value length: %d", kvp->val_len);
			KVP_HEXDUMP(kvp->val, kvp->val_len, "Invalid value");
			break;
		}

		next = newline + 1;
	} while (0);

	return next;
}

/**
 * @retval negative on error, otherwise number of key-value pairs.
 */
static int parse_kvp_file(const lcz_kvp_cfg_t *cfg, const char *str, int pairs, lcz_kvp_t *kv)
{
	char *next = (char *)str; /* start */
	char *end = next + cfg->max_file_size;
	int i;

	for (i = 0; i < pairs; i++) {
		if (next >= end) {
			LOG_ERR("Unexpected end of kvp file");
			pairs = -EPERM;
			break;
		}

		next = parse_kvp(cfg, next, &kv[i]);
		if (next == NULL) {
			pairs = -EINVAL;
			break;
		}
	}

	return pairs;
}

/**
 * @brief Read the text file from the filesystem into a buffer in RAM.
 *
 * @param[in] ctx File context (clear or encrypted)
 * @param[in] fname Absolute file name of the file to read
 * @param[in] fstr Pointer to where the pointer to the data should be stored
 * @param[in] fsize Size of the file on disk
 *
 * @retval negative on error, otherwise size of string with carriage returns
 * removed.
 */
static int read_text(const func_context_t *ctx, const char *fname, char **fstr, size_t fsize)
{
	int r = 0;
	int stripped_len = 0;
	bool comment;
	int i;
	int j;

	/* Validate the input parameters */
	if (fname == NULL || fstr == NULL) {
		LOG_ERR("read text %s: Invalid parameters", ctx->msg);
		r = -EINVAL;
	}

	/* Allocate memory for the file contents */
	if (r == 0) {
		*fstr = k_malloc(fsize + 1);
		if (*fstr == NULL) {
			LOG_ERR("read_text %s: Could not allocate %d bytes for %s", ctx->msg,
				fsize + 1, fname);
			r = -ENOMEM;
		}
	}

	/* Read the entire file into memory */
	if (r == 0) {
		r = ctx->read(fname, *fstr, fsize);
		if (r != fsize) {
			LOG_ERR("read_text %s: Could not read file %s: %d", ctx->msg, fname, r);
			if (r >= 0) {
				r = -EPERM;
			}
		} else {
			r = 0;
		}
	}

	/* Strip the CRs from the file */
	if (r == 0) {
		i = 0;
		j = 0;
		while (i < fsize) {
			if ((*fstr)[i] != CR_CHAR) {
				(*fstr)[j] = (*fstr)[i];
				j++;
			}
			i++;
		}
		/* Clear the remainder of the input string */
		memset((*fstr) + j, 0, i - j);
		stripped_len = j;
	}

	/* Strip the comments */
	if (r == 0) {
		i = 0;
		j = 0;
		comment = false;
		while (i < stripped_len) {
			if ((*fstr)[i] == COMMENT_CHAR) {
				comment = true;
			}

			if (!comment) {
				(*fstr)[j] = (*fstr)[i];
				j++;
			}

			/* Remove entire comment line */
			if ((*fstr)[i] == EOL_CHAR) {
				comment = false;
			}

			i++;
		}
		/* Clear the remainder of the input string */
		memset((*fstr) + j, 0, i - j);
		stripped_len = j;
	}

	/* Return the stripped length or the error code */
	if (r == 0) {
		return stripped_len;
	} else {
		return r;
	}
}

static bool valid_cfg(const lcz_kvp_cfg_t *cfg)
{
	if (cfg == NULL) {
		return false;
	}

	if (cfg->max_file_size < 0 || cfg->max_key_len < 0 || cfg->max_val_len < 0) {
		return false;
	}

	if (DELIMITER == EOL_CHAR) {
		return false;
	}

	return true;
}

static bool valid_kvp(const lcz_kvp_cfg_t *cfg, const lcz_kvp_t *kvp)
{
	if (cfg == NULL || kvp == NULL || kvp->key == NULL || kvp->val == NULL) {
		return false;
	}

	if (kvp->key_len > cfg->max_key_len || kvp->val_len > cfg->max_val_len) {
		return false;
	}

	return true;
}

/**
 * @return ssize_t length of line or negative error code
 */
static ssize_t space_avail(const lcz_kvp_cfg_t *cfg, const lcz_kvp_t *kvp, size_t start_len)
{
	size_t overhead = sizeof(DELIMITER) + sizeof(EOL_CHAR) + sizeof(TERMINATOR);
	size_t line_len = kvp->key_len + kvp->val_len + overhead;

	if ((start_len + line_len) < cfg->max_file_size) {
		return line_len;
	} else {
		return -ENOMEM;
	}
}

static int append_kvp_line(const lcz_kvp_cfg_t *cfg, const lcz_kvp_t *kvp, char *str)
{
	ssize_t length = -EINVAL;
	ssize_t line_len = -EINVAL;

	do {
		if (str == NULL) {
			break;
		}

		if (!valid_kvp(cfg, kvp)) {
			break;
		}

		length = strlen(str);
		line_len = space_avail(cfg, kvp, length);
		if (line_len < 0) {
			break;
		}

		APPEND(kvp->key, kvp->key_len);
		APPEND_CHAR(DELIMITER);
		APPEND(kvp->val, kvp->val_len);
		APPEND_CHAR(EOL_CHAR);
		str[length] = TERMINATOR;
		/* Don't add terminator to length or strlen won't match */

	} while (0);

	LOG_VRB("total length: %d line length: %d", length, line_len);

	return line_len;
}

static func_context_t get_func_context(bool encrypted)
{
	func_context_t ctx;

	if (encrypted) {
#if defined(CONFIG_FSU_ENCRYPTED_FILES)
		ctx.get_size = efs_get_file_size;
		ctx.read = efs_read;
		ctx.write = efs_write;
		ctx.append = efs_append;
		ctx.msg = "encrypted";
#else
		ctx.get_size = NULL;
		ctx.read = NULL;
		ctx.write = NULL;
		ctx.append = NULL;
		ctx.msg = "encrypted key-value pair files not supported";
		LOG_ERR("%s", ctx.msg);
#endif
	} else {
		ctx.get_size = fsu_get_file_size_abs;
		ctx.read = fsu_read_abs;
		ctx.write = fsu_write_abs;
		ctx.append = fsu_append_abs;
		ctx.msg = "cleartext";
	}

	return ctx;
}
