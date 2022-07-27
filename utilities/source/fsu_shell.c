/**
 * @file fsu_shell.c
 * @brief Test file system utilies.
 * @note There is duplication between these functions and fs shell.
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <shell/shell.h>
#include <init.h>
#include <stdio.h>
#include <stdlib.h>

#include "file_system_utilities.h"
#if defined(CONFIG_FSU_ENCRYPTED_FILES)
#include "encrypted_file_storage.h"
#endif

/**************************************************************************************************/
/* Local Data Definitions                                                                         */
/**************************************************************************************************/
#if defined(CONFIG_FSU_SHELL_ALLOW_CHANGE)
static int macro_status;
#endif

static char mount_point[FSU_MAX_ABS_PATH_SIZE];

/**************************************************************************************************/
/* Local Constant, Macro and Type Definitions                                                     */
/**************************************************************************************************/
#if defined(CONFIG_FSU_SHELL_ALLOW_CHANGE)
static const char SET_DESCRIPTION[] = "Set a parameter:\r\n"
				      "mount_point ";
#endif

#define FSUS_SET_STRING(field, name, src)                                                          \
	do {                                                                                       \
		size_t max_size = sizeof(field);                                                   \
		if (strcmp(name, STRINGIFY(field)) == 0) {                                         \
			memset(field, 0, max_size);                                                \
			strncpy(field, src, max_size - 1);                                         \
			FSUS_PRINT_STRING(field);                                                  \
			macro_status = 0;                                                          \
		}                                                                                  \
	} while (0)

#define FSUS_SET_NUMBER(field, name, src)                                                          \
	do {                                                                                       \
		if (strcmp(name, STRINGIFY(field)) == 0) {                                         \
			field = (int32_t)strtol(src, NULL, 0);                                     \
			FSUS_PRINT_NUMBER(field);                                                  \
			macro_status = 0;                                                          \
		}                                                                                  \
	} while (0)

#define FSUS_PRINT_STRING(field) shell_print(shell, "%s: %s", STRINGIFY(field), field)

#define FSUS_PRINT_NUMBER(field) shell_print(shell, "%s: %u", STRINGIFY(field), field)

#define FSUS_ALLOW_DELETE_ALL_STR "-f"

/**************************************************************************************************/
/* Local Function Prototypes                                                                      */
/**************************************************************************************************/
#if defined(CONFIG_FSU_SHELL_ALLOW_CHANGE)
static int fsu_set_cmd(const struct shell *shell, size_t argc, char **argv);
#endif
static int fsu_query_cmd(const struct shell *shell, size_t argc, char **argv);
static int fsu_ls_cmd(const struct shell *shell, size_t argc, char **argv);
static int fsu_sha_cmd(const struct shell *shell, size_t argc, char **argv);
static int fsu_crc_cmd(const struct shell *shell, size_t argc, char **argv);
#if defined(CONFIG_FSU_SHELL_ALLOW_CHANGE)
static int fsu_del_cmd(const struct shell *shell, size_t argc, char **argv);
#endif
static int fsu_dump_cmd(const struct shell *shell, size_t argc, char **argv);
#if defined(CONFIG_FSU_ENCRYPTED_FILES)
static int fsu_enc_create_cmd(const struct shell *shell, size_t argc, char **argv);
static int fsu_enc_append_cmd(const struct shell *shell, size_t argc, char **argv);
static int fsu_enc_dump_cmd(const struct shell *shell, size_t argc, char **argv);
static int fsu_enc_sha256_cmd(const struct shell *shell, size_t argc, char **argv);
#endif

static int fsu_shell_init(const struct device *device);

/**************************************************************************************************/
/* Global Function Definitions                                                                    */
/**************************************************************************************************/
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_fsu,
#if defined(CONFIG_FSU_SHELL_ALLOW_CHANGE)
	SHELL_CMD(set, NULL, SET_DESCRIPTION, fsu_set_cmd),
#endif
	SHELL_CMD(query, NULL, "List shell parameters", fsu_query_cmd),
	SHELL_CMD(ls, NULL, "List files (with size)", fsu_ls_cmd),
	SHELL_CMD(sha, NULL, "Get SHA256 of file(s)", fsu_sha_cmd),
	SHELL_CMD(crc, NULL, "Get CRC32 checksum of file(s)", fsu_crc_cmd),
#if defined(CONFIG_FSU_SHELL_ALLOW_CHANGE)
	SHELL_CMD(del, NULL, "Delete file(s) [-f is delete all]", fsu_del_cmd),
#endif
	SHELL_CMD(dump, NULL, "Dump the contents of a file", fsu_dump_cmd),
#if defined(CONFIG_FSU_ENCRYPTED_FILES)
	SHELL_CMD(enc_create, NULL, "Create an encrypted file", fsu_enc_create_cmd),
	SHELL_CMD(enc_append, NULL, "Append to an encrypted file", fsu_enc_append_cmd),
	SHELL_CMD(enc_dump, NULL, "Dump the contents of an encrypted file", fsu_enc_dump_cmd),
	SHELL_CMD(enc_sha, NULL, "Calculate hash of an encrypted file", fsu_enc_sha256_cmd),
#endif
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(fsu, &sub_fsu, "File System Utilities", NULL);

SYS_INIT(fsu_shell_init, APPLICATION, 99);

/**************************************************************************************************/
/* Local Function Definitions                                                                     */
/**************************************************************************************************/
#if defined(CONFIG_FSU_SHELL_ALLOW_CHANGE)
static int fsu_set_cmd(const struct shell *shell, size_t argc, char **argv)
{
	if ((argc == 3) && (argv[1] != NULL) && (argv[2] != NULL)) {
		macro_status = -1;
		FSUS_SET_STRING(mount_point, argv[1], argv[2]);
		if (macro_status != 0) {
			shell_error(shell, "parameter %s not found", argv[1]);
		}
	} else {
		shell_error(shell, "Unexpected parameters");
		return -EINVAL;
	}
	return 0;
}
#endif

static int fsu_query_cmd(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	FSUS_PRINT_STRING(mount_point);
	return 0;
}

static char *fsu_name_handler(size_t argc, char **argv)
{
	if ((argc == 2) && (argv[1] != NULL) && (strlen(argv[1]) > 0)) {
		return argv[1];
	} else {
		return FSU_EMPTY_STRING;
	}
}

static int fsu_ls_cmd(const struct shell *shell, size_t argc, char **argv)
{
	char *name = fsu_name_handler(argc, argv);

	size_t count = 0;
	struct fs_dirent *pEntries = fsu_find(mount_point, name, &count, FS_DIR_ENTRY_FILE);
	shell_print(shell, "Found %u matching files", count);

	size_t i;
	for (i = 0; i < count; i++) {
		shell_info(shell, "%08u %s", pEntries[i].size, pEntries[i].name);
	}
	fsu_free_found(pEntries);
	return 0;
}

static int fsu_sha_cmd(const struct shell *shell, size_t argc, char **argv)
{
	char *name = fsu_name_handler(argc, argv);

	uint8_t hash[FSU_HASH_SIZE];
	ssize_t status = 0;
	size_t count = 0;
	struct fs_dirent *pEntries = fsu_find(mount_point, name, &count, FS_DIR_ENTRY_FILE);
	if (count == 0) {
		status = -ENOENT;
		shell_print(shell, "No files found");
	} else {
		shell_print(shell, "Found %u matching files", count);
		size_t i = 0;
		while (i < count) {
			status = fsu_sha256(hash, mount_point, pEntries[i].name, pEntries[i].size);
			shell_print(shell, "SHA-256 of %s status: %d", pEntries[i].name, status);

			if (status == 0) {
				shell_hexdump(shell, hash, FSU_HASH_SIZE);
			} else {
				break;
			}
			i += 1;
		}
	}
	fsu_free_found(pEntries);
	return 0;
}

static int fsu_crc_cmd(const struct shell *shell, size_t argc, char **argv)
{
	char *name = fsu_name_handler(argc, argv);

	uint32_t checksum = 0;
	ssize_t status = 0;
	size_t count = 0;
	struct fs_dirent *pEntries = fsu_find(mount_point, name, &count, FS_DIR_ENTRY_FILE);
	if (count == 0) {
		status = -ENOENT;
		shell_print(shell, "No files found");
	} else {
		shell_print(shell, "Found %u matching files", count);
		size_t i = 0;
		while (i < count) {
			status = fsu_crc32(&checksum, mount_point, pEntries[i].name,
					   pEntries[i].size);

			if (status == 0) {
				shell_info(shell, "%08x %s", checksum, pEntries[i].name);
			} else {
				shell_print(shell, "CRC32 of %s status: %d", pEntries[i].name,
					    status);
				break;
			}
			i += 1;
		}
	}
	fsu_free_found(pEntries);
	return 0;
}

#if defined(CONFIG_FSU_SHELL_ALLOW_CHANGE)
static int fsu_del_cmd(const struct shell *shell, size_t argc, char **argv)
{
	/* -f is used to make it harder to accidentally delete all */
	char *name;
	if ((argc == 2) && (argv[1] != NULL) && (strlen(argv[1]) > 0)) {
		if (strcmp(argv[1], FSUS_ALLOW_DELETE_ALL_STR) == 0) {
			name = FSU_EMPTY_STRING;
		} else {
			name = argv[1];
		}
	} else {
		shell_error(shell, "Unexpected parameters");
		return -EINVAL;
	}

	int count = fsu_delete_files(mount_point, name);
	shell_print(shell, "Deleted %u matching files", count);
	return 0;
}
#endif

static int fsu_dump_cmd(const struct shell *shell, size_t argc, char **argv)
{
	char *abs_path = fsu_name_handler(argc, argv);
	uint8_t *contents;
	ssize_t status = 0;
	ssize_t size;

	size = fsu_get_file_size_abs(abs_path);
	if (size < 0) {
		shell_print(shell, "Failed to read size of %s: %d", abs_path, size);
	} else {
		contents = (uint8_t *)k_malloc(size);
		if (contents != NULL) {
			status = fsu_read_abs(abs_path, contents, size);
			shell_print(shell, "File %s contents (%d bytes): ", abs_path, size);
			if (status > 0) {
				shell_hexdump(shell, contents, size);
			} else {
				shell_print(shell, "Read of %d bytes failed: %d", size, status);
			}
			k_free(contents);
		}
	}
	return 0;
}

#if defined(CONFIG_FSU_ENCRYPTED_FILES)
static int fsu_enc_create_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int status;
	uint8_t contents[256];
	int i;

	if (argc != 2) {
		shell_print(shell, "Need a filename");
		return 0;
	}

	/* Fill the block with known data */
	for (i = 0; i < sizeof(contents); i++) {
		contents[i] = i;
	}

	shell_print(shell, "Writing data to file:");
	shell_hexdump(shell, contents, sizeof(contents));

	status = efs_write(argv[1], contents, sizeof(contents));
	shell_print(shell, "Write result to %s: %d", argv[1], status);

	return 0;
}

static int fsu_enc_append_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int status;
	uint8_t contents[256];
	int i;

	if (argc != 2) {
		shell_print(shell, "Need a filename");
		return 0;
	}

	/* Fill the block with known data */
	for (i = 0; i < sizeof(contents); i++) {
		contents[i] = i;
	}

	shell_print(shell, "Writing data to file:");
	shell_hexdump(shell, contents, sizeof(contents));

	status = efs_append(argv[1], contents, sizeof(contents));
	shell_print(shell, "Append result to %s: %d", argv[1], status);

	return 0;
}

static int fsu_enc_dump_cmd(const struct shell *shell, size_t argc, char **argv)
{
	char *abs_path = fsu_name_handler(argc, argv);
	uint8_t *contents;
	ssize_t status = 0;
	ssize_t file_size = 0;
	file_size = efs_get_file_size(abs_path);
	if (file_size > 0) {
		contents = (uint8_t *)k_malloc(file_size);
		if (contents != NULL) {
			status = efs_read(abs_path, contents, file_size);
			shell_print(shell, "File %s contents (%d bytes): ", abs_path, file_size);
			if (status > 0) {
				shell_hexdump(shell, contents, file_size);
			} else {
				shell_print(shell, "Failed %d", status);
			}
			k_free(contents);
		}
	} else {
		shell_print(shell, "Failed to read file size of %s: %d", abs_path, file_size);
	}
	return 0;
}

static int fsu_enc_sha256_cmd(const struct shell *shell, size_t argc, char **argv)
{
	char abs_path[FSU_MAX_ABS_PATH_SIZE];
	char *name = fsu_name_handler(argc, argv);
	uint8_t hash[FSU_HASH_SIZE];
	ssize_t status = 0;
	ssize_t file_size = 0;
	size_t count = 0;
	struct fs_dirent *pEntries = fsu_find(mount_point, name, &count, FS_DIR_ENTRY_FILE);
	if (count == 0) {
		status = -ENOENT;
		shell_print(shell, "No files found");
	} else {
		shell_print(shell, "Found %u matching files", count);
		size_t i = 0;
		while (i < count) {
			(void)fsu_build_full_name(abs_path, sizeof(abs_path), mount_point, name);
			file_size = efs_get_file_size(abs_path);
			if (file_size > 0) {
				status = efs_sha256(hash, abs_path, file_size);
				shell_print(shell, "File %s hash: ", pEntries[i].name);

				if (status == 0) {
					shell_hexdump(shell, hash, FSU_HASH_SIZE);
				} else {
					shell_print(shell, "Failed %d", status);
					break;
				}
			} else {
				shell_print(shell, "Failed to read file size of %s: %d", abs_path,
					    file_size);
			}
			i += 1;
		}
	}
	fsu_free_found(pEntries);
	return 0;
}
#endif

static int fsu_shell_init(const struct device *device)
{
	ARG_UNUSED(device);
	strcpy(mount_point, CONFIG_FSU_MOUNT_POINT);
	return 0;
}
