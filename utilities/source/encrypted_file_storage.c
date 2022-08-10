/**
 * @file encrypted_file_storage.c
 *
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: LicenseRef-LairdConnectivity-Clause
 *
 * Encrypted files are stored using a block size of 1024 bytes (EFS_FILE_BLOCK_SIZE). Of this
 * 1024 bytes, 960 bytes are used to store encrypted "user" data (EFS_USER_BLOCK_SIZE). The
 * remaining 64 bytes are divided up into a block header of 48 bytes and a MAC of 16 bytes.
 * The header contains the initialization vector, the block number in the file, the number of
 * user bytes of data held by the block, and a SHA256 hash of the file name.
 *
 * A portion of the block header (defined in struct auth_data) is authenticated along with the
 * encrypted user data using the 16 byte MAC. This is done in order to prevent a block from
 * one file from being substituted into another file.
 *
 * File blocks are always full size. Plaintext user data is padded up to the 960 byte size to
 * ensure this. The data size in the header is always the actual number of real user bytes, not
 * including padding.
 */

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_efs, CONFIG_FSU_LOG_LEVEL);

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <zephyr.h>
#include <init.h>
#include <psa/crypto.h>
#include <lcz_hw_key.h>
#include "file_system_utilities.h"
#include "encrypted_file_storage.h"

/**************************************************************************************************/
/* Local Constant, Macro and Type Definitions                                                     */
/**************************************************************************************************/
#define FSE_FILE_NAME_HASH_ALG PSA_ALG_SHA_256

struct auth_data {
	uint16_t block_number; /* 960 bytes per block = maximum file size of 60 Mbytes */
	uint16_t block_size; /* values between 1 and 960 bytes */
	uint8_t file_name_hash[PSA_HASH_LENGTH(FSE_FILE_NAME_HASH_ALG)];
};

struct efs_block_header {
	uint8_t iv[LCZ_HW_KEY_IV_LEN];
	struct auth_data auth_data;
};

#define EFS_FILE_BLOCK_SIZE 1024
#define EFS_USER_BLOCK_SIZE                                                                        \
	(EFS_FILE_BLOCK_SIZE - (sizeof(struct efs_block_header) + LCZ_HW_KEY_MAC_LEN))

#define EFS_FILE_BLOCK_ENC_OFFSET (sizeof(struct efs_block_header))

/**************************************************************************************************/
/* Local Function Prototypes                                                                      */
/**************************************************************************************************/
static int file_name_hash_gen(const char *abs_path, uint8_t *hash, uint8_t hash_len);
static int file_name_hash_comp(const char *abs_path, uint8_t *hash, uint8_t hash_len);
static int encrypt_block(struct efs_block_header *hdr, uint8_t *user_data, ssize_t user_data_len,
			 uint8_t *out);
static int decrypt_block(const char *abs_path, int block_number, struct efs_block_header *hdr,
			 uint8_t *in_data, int in_data_len, uint8_t *out_data, int out_data_len);
static int lcz_enc_fs_init(const struct device *device);

/**************************************************************************************************/
/* Global Function Definitions                                                                    */
/**************************************************************************************************/
bool efs_is_encrypted_path(const char *abs_path)
{
	char simple_path[FSU_MAX_ABS_PATH_SIZE + 1];

	/* Simplify the input path */
	if (fsu_simplify_path(abs_path, simple_path) < 0) {
		LOG_ERR("efs_is_encrypted_path: Invalid path: %s", log_strdup(abs_path));

		/* To be on the safe side, say "true" to an invalid path */
		return true;
	}

	/* Compare against the encrypted file path */
	if (strncmp(simple_path, CONFIG_FSU_ENCRYPTED_FILE_PATH,
		   strlen(CONFIG_FSU_ENCRYPTED_FILE_PATH)) == 0) {
		return true;
	}
	return false;
}

int efs_write(const char *abs_path, uint8_t *data, size_t size)
{
	int ret = 0;
	int ret2;
	struct fs_file_t f;

	/* Validate the input parameters */
	if (abs_path == NULL) {
		LOG_ERR("efs_write: invalid path");
		ret = -EINVAL;
	}

	/* Open the file for writing */
	fs_file_t_init(&f);
	if (ret == 0) {
		ret = fs_open(&f, abs_path, FS_O_WRITE | FS_O_CREATE);
		if (ret < 0) {
			LOG_ERR("efs_read: fs_open failed %d", ret);
		}
	}

	/* Empty the file */
	if (ret == 0) {
		ret = fs_truncate(&f, 0);
		if (ret < 0) {
			LOG_ERR("efs_write: Could not truncate file: %d", ret);
		}
	}

	/* Close the file */
	ret2 = fs_close(&f);
	if (ret2 < 0) {
		LOG_ERR("efs_write: Could not close file: %d", ret2);
		if (ret == 0) {
			ret = ret2;
		}
	}

	/* Now append the new data */
	if (ret == 0) {
		ret = efs_append(abs_path, data, size);
	}

	return ret;
}

int efs_append(const char *abs_path, uint8_t *data, size_t size)
{
	char simple_path[FSU_MAX_ABS_PATH_SIZE + 1];
	int ret = 0;
	int ret2;
	ssize_t file_size = 0;
	int this_size;
	struct fs_file_t f;
	uint8_t *file_block = NULL;
	uint8_t *user_block = NULL;
	int block_offset = 0;
	int copied = 0;
	struct efs_block_header *hdr = NULL;

	/* Validate input parameters */
	if (abs_path == NULL) {
		LOG_ERR("efs_append: invalid path");
		ret = -EINVAL;
	} else if (size != 0 && data == NULL) {
		LOG_ERR("efs_append: null data pointer for size %d", size);
		ret = -EINVAL;
	}

	/* Remove any extra slashes in the path */
	if (ret == 0) {
		ret = fsu_simplify_path(abs_path, simple_path);
		if (ret < 0) {
			LOG_ERR("efs_append: Invalid input path: %s", log_strdup(abs_path));
		} else {
			ret = 0;
		}
	}

	/* Check current file size */
	if (ret == 0) {
		file_size = fsu_get_file_size_abs(simple_path);
		if (file_size < 0) {
			/* Assume file doesn't exist */
			file_size = 0;
		}
	}

	/* Encrypted files must be a multiple of the block size */
	if (ret == 0) {
		if ((file_size % EFS_FILE_BLOCK_SIZE) != 0) {
			LOG_ERR("efs_append: File is not multiple of block size");
			ret = -EINVAL;
		}
	}

	/* Allocate memory for the file block */
	if (ret == 0) {
		file_block = (uint8_t *)k_malloc(EFS_FILE_BLOCK_SIZE);
		if (file_block == NULL) {
			LOG_ERR("efs_append: Could not allocate memory for the file block");
			ret = -ENOMEM;
		}
		hdr = (struct efs_block_header *)file_block;
	}

	/* Allocate memory for the output block */
	if (ret == 0) {
		user_block = (uint8_t *)k_malloc(EFS_USER_BLOCK_SIZE);
		if (user_block == NULL) {
			LOG_ERR("efs_append: Could not allocate memory for the user block");
			ret = -ENOMEM;
		}
	}

	/* Open the file for read and write */
	fs_file_t_init(&f);
	if (ret == 0) {
		ret = fs_open(&f, simple_path, FS_O_RDWR | FS_O_CREATE);
		if (ret < 0) {
			LOG_ERR("efs_append: fs_open failed %d", ret);
		}
	}

	/* If the file is not empty, try to fill up the last block first */
	if (ret == 0 && size > 0 && file_size > 0) {
		/* Seek to the start of the last encrypted block */
		block_offset = (file_size / EFS_FILE_BLOCK_SIZE) - 1;
		ret = fs_seek(&f, block_offset * EFS_FILE_BLOCK_SIZE, FS_SEEK_SET);
		if (ret < 0) {
			LOG_ERR("efs_append: seek failed to block %d: %d", block_offset, ret);
		}

		/* Read the encrypted block */
		if (ret == 0) {
			ret = fs_read(&f, file_block, EFS_FILE_BLOCK_SIZE);
			if (ret < 0) {
				LOG_ERR("efs_append: read failed for block %d: %d", block_offset,
					ret);
			} else if (ret != EFS_FILE_BLOCK_SIZE) {
				LOG_ERR("efs_append: Read only returned %d bytes", ret);
				ret = -EIO;
			} else {
				/* Good read */
				ret = 0;
			}
		}

		/* Decrypt the block */
		if (ret == 0) {
			ret = decrypt_block(simple_path, block_offset, hdr,
					    file_block + EFS_FILE_BLOCK_ENC_OFFSET,
					    EFS_FILE_BLOCK_SIZE - EFS_FILE_BLOCK_ENC_OFFSET,
					    user_block, EFS_USER_BLOCK_SIZE);
			if (ret < 0) {
				LOG_ERR("efs_append: decrypt failed for block %d: %d", block_offset,
					ret);
			}
		}

		if (ret == 0) {
			/* If there is space, add all or part of the new data to the end of the block */
			if (hdr->auth_data.block_size < EFS_USER_BLOCK_SIZE) {
				/* Figure out how much data can fit */
				this_size = size;
				if (this_size > (EFS_USER_BLOCK_SIZE - hdr->auth_data.block_size)) {
					this_size = EFS_USER_BLOCK_SIZE - hdr->auth_data.block_size;
				}

				/* Copy what we can */
				memcpy(user_block + hdr->auth_data.block_size, data, this_size);

				/* Update pointers and lengths */
				data += this_size;
				size -= this_size;
				copied += this_size;
				hdr->auth_data.block_size += this_size;

				/* Zero out the rest */
				if (hdr->auth_data.block_size != EFS_USER_BLOCK_SIZE) {
					memset(user_block + hdr->auth_data.block_size, 0,
					       EFS_USER_BLOCK_SIZE - hdr->auth_data.block_size);
				}

				/* Re-encrypt the block */
				ret = encrypt_block(hdr, user_block, EFS_USER_BLOCK_SIZE,
						    file_block + EFS_FILE_BLOCK_ENC_OFFSET);
				if (ret < 0) {
					LOG_ERR("efs_append: encrypt failed for block %d: %d",
						block_offset, ret);
				}

				/* Reset the file pointer */
				if (ret == 0) {
					ret = fs_seek(&f, block_offset * EFS_FILE_BLOCK_SIZE,
						      FS_SEEK_SET);
					if (ret < 0) {
						LOG_ERR("efs_append: seek failed to block %d: %d",
							block_offset, ret);
					}
				}

				/* Write the block back to the file */
				if (ret == 0) {
					ret = fs_write(&f, file_block, EFS_FILE_BLOCK_SIZE);
					if (ret < 0) {
						LOG_ERR("efs_append: write failed to block %d: %d",
							block_offset, ret);
					} else if (ret != EFS_FILE_BLOCK_SIZE) {
						LOG_ERR("efs_append: write only wrote %d bytes",
							ret);
						ret = -EIO;
					} else {
						/* Good write */
						ret = 0;
					}
				}
				/* File pointer should now be at the end of the file */

				/* Update block offset */
				block_offset++;
			}
		}
	}

	/* Start to fill in the block header */
	if (ret == 0) {
		ret = file_name_hash_gen(simple_path, hdr->auth_data.file_name_hash,
					 sizeof(hdr->auth_data.file_name_hash));
		if (ret < 0) {
			LOG_ERR("efs_append: Couldn't hash filename: %d", ret);
		}
	}

	/* Chunk the data into EFS_USER_BLOCK_SIZE bytes to write into the file */
	while (ret == 0 && size > 0) {
		this_size = size;
		if (this_size > EFS_USER_BLOCK_SIZE) {
			this_size = EFS_USER_BLOCK_SIZE;
		}

		/* Populate the header for this block */
		hdr->auth_data.block_number = block_offset;
		hdr->auth_data.block_size = this_size;

		/* Encrypt the block */
		if (this_size == EFS_USER_BLOCK_SIZE) {
			/* For a full block, don't make a copy of the user data first */
			ret = encrypt_block(hdr, data, this_size,
					    file_block + EFS_FILE_BLOCK_ENC_OFFSET);
		} else {
			/* For a block needing padding, copy the user data and then pad */
			memcpy(user_block, data, this_size);
			memset(user_block + this_size, 0, EFS_USER_BLOCK_SIZE - this_size);
			ret = encrypt_block(hdr, user_block, EFS_USER_BLOCK_SIZE,
					    file_block + EFS_FILE_BLOCK_ENC_OFFSET);
		}
		if (ret < 0) {
			LOG_ERR("efs_append: encrypt failed for new block %d: %d", block_offset,
				ret);
		}

		/* Write the block back to the file */
		if (ret == 0) {
			ret = fs_write(&f, file_block, EFS_FILE_BLOCK_SIZE);
			if (ret < 0) {
				LOG_ERR("efs_append: write failed to new block %d: %d",
					block_offset, ret);
			} else if (ret != EFS_FILE_BLOCK_SIZE) {
				LOG_ERR("efs_append: new write only wrote %d bytes", ret);
				ret = -EIO;
			} else {
				/* Good write */
				ret = 0;
			}
		}

		/* Update pointers/counters */
		if (ret == 0) {
			block_offset++;
			data += this_size;
			size -= this_size;
			copied += this_size;
		}
	}

	/* Close the file */
	ret2 = fs_close(&f);
	if (ret2 < 0) {
		LOG_ERR("efs_append: Could not close file: %d", ret2);
		if (ret >= 0) {
			ret = ret2;
		}
	}

	/* Free any memory that we allocated */
	if (file_block != NULL) {
		memset(file_block, 0, EFS_FILE_BLOCK_SIZE);
		k_free(file_block);
	}
	if (user_block != NULL) {
		memset(user_block, 0, EFS_USER_BLOCK_SIZE);
		k_free(user_block);
	}

	/* Return the error or the number of bytes written */
	if (ret == 0) {
		ret = copied;
	}
	return ret;
}

ssize_t efs_read(const char *abs_path, uint8_t *data, size_t size)
{
	return efs_read_block(abs_path, 0, data, size);
}

ssize_t efs_read_block(const char *abs_path, int offset, uint8_t *data, size_t size)
{
	char simple_path[FSU_MAX_ABS_PATH_SIZE + 1];
	int block_num;
	int block_offset;
	int this_size;
	int copied = 0;
	ssize_t file_size = 0;
	uint8_t *file_block = NULL;
	uint8_t *user_block = NULL;
	struct efs_block_header *hdr;
	struct fs_file_t f;
	int ret = 0;
	int ret2;

	/* Validate the input parameters */
	if (abs_path == NULL || data == NULL || size == 0) {
		ret = -EINVAL;
	}

	/* Remove any extra slashes in the path */
	if (ret == 0) {
		ret = fsu_simplify_path(abs_path, simple_path);
		if (ret < 0) {
			LOG_ERR("efs_read_block: Invalid input path: %s", log_strdup(abs_path));
		} else {
			ret = 0;
		}
	}

	/* Compute which encrypted block the offset should be in */
	block_num = offset / EFS_USER_BLOCK_SIZE;
	block_offset = offset % EFS_USER_BLOCK_SIZE;

	/* Read the file size */
	if (ret == 0) {
		file_size = fsu_get_file_size_abs(simple_path);
		if (file_size < 0) {
			ret = file_size;
		}
	}

	/* Make sure that the file is a mulitple of the encrypted block size */
	if (ret == 0) {
		if ((file_size % EFS_FILE_BLOCK_SIZE) != 0) {
			LOG_ERR("efs_read_block: File is not multiple of block size (%d)",
				file_size);
			ret = -EINVAL;
		}
	}

	/* Make sure that the file is large enough to contain the requested block */
	if (ret == 0) {
		if (((block_num * EFS_FILE_BLOCK_SIZE) + EFS_FILE_BLOCK_SIZE) > file_size) {
			LOG_ERR("efs_read_block: File is not large enough (%d) for requested offset (%d)",
				file_size, offset);
			ret = -EINVAL;
		}
	}

	/* Open the file */
	fs_file_t_init(&f);
	if (ret == 0) {
		ret = fs_open(&f, simple_path, FS_O_READ);
		if (ret < 0) {
			LOG_ERR("efs_read_block: fs_open failed %d", ret);
		}
	}

	/* Allocate memory for the file block */
	file_block = (uint8_t *)k_malloc(EFS_FILE_BLOCK_SIZE);
	if (file_block == NULL) {
		LOG_ERR("efs_read_block: Could not allocate memory for the file block");
		ret = -ENOMEM;
	}

	/* Allocate memory for the output block */
	user_block = (uint8_t *)k_malloc(EFS_USER_BLOCK_SIZE);
	if (user_block == NULL) {
		LOG_ERR("efs_read_block: Could not allocate memory for the user block");
		ret = -ENOMEM;
	}

	/* Seek to the first block */
	ret = fs_seek(&f, block_num * EFS_FILE_BLOCK_SIZE, FS_SEEK_SET);
	if (ret < 0) {
		LOG_ERR("efs_read_block: seek failed to block %d", block_num);
	}

	/* Read the block(s) of the file */
	while (ret == 0 && size > 0 &&
	       ((block_num * EFS_FILE_BLOCK_SIZE) + EFS_FILE_BLOCK_SIZE) <= file_size) {
		/* Seek to the start of the encrypted block */

		/* Read the encrypted block */
		if (ret == 0) {
			ret = fs_read(&f, file_block, EFS_FILE_BLOCK_SIZE);
			if (ret < 0) {
				LOG_ERR("efs_read_block: read failed for block %d", block_num);
			} else if (ret != EFS_FILE_BLOCK_SIZE) {
				LOG_ERR("efs_read_block: Read only returned %d bytes", ret);
				ret = -EIO;
			} else {
				/* Good read */
				ret = 0;
			}
		}

		/* Decrypt the block */
		if (ret == 0) {
			hdr = (struct efs_block_header *)file_block;
			ret = decrypt_block(simple_path, block_num, hdr,
					    file_block + EFS_FILE_BLOCK_ENC_OFFSET,
					    EFS_FILE_BLOCK_SIZE - EFS_FILE_BLOCK_ENC_OFFSET,
					    user_block, EFS_USER_BLOCK_SIZE);
			if (ret < 0) {
				LOG_ERR("efs_read_block: decrypt failed for block %d", block_num);
			}
		}

		/* Copy the decrypted data to the user's buffer */
		if (ret == 0) {
			/* Limit the data size to what is in this block */
			this_size = size;
			if (this_size > hdr->auth_data.block_size) {
				this_size = hdr->auth_data.block_size;
			}

			/* Adjust size taking the offset into account */
			this_size -= block_offset;

			/* Copy the data into the caller's buffer */
			memcpy(data, user_block + block_offset, this_size);

			/* Update pointers/counters */
			block_offset = 0;
			data += this_size;
			size -= this_size;
			copied += this_size;
		}

		/* Move to the next block */
		if (ret == 0 && size > 0) {
			if (hdr->auth_data.block_size == EFS_USER_BLOCK_SIZE) {
				/* Try the next block only if the current block was "full" */
				block_num++;
			} else {
				/* There shouldn't be another block after this one */
				break;
			}
		}
	}

	/* Close the file */
	ret2 = fs_close(&f);
	if (ret2 < 0) {
		LOG_ERR("efs_read_block: Could not close file: %d", ret2);
		if (ret >= 0) {
			ret = ret2;
		}
	}

	/* Free any memory that we allocated */
	if (file_block != NULL) {
		memset(file_block, 0, EFS_FILE_BLOCK_SIZE);
		k_free(file_block);
	}
	if (user_block != NULL) {
		memset(user_block, 0, EFS_USER_BLOCK_SIZE);
		k_free(user_block);
	}

	/* Return error or the number of bytes copied */
	if (ret == 0) {
		ret = copied;
	}
	return ret;
}

ssize_t efs_get_file_size(const char *abs_path)
{
	char simple_path[FSU_MAX_ABS_PATH_SIZE + 1];
	ssize_t file_size = 0;
	int num_blocks;
	int block_offset;
	int ret = 0;
	int ret2;
	uint8_t *file_block = NULL;
	uint8_t *user_block = NULL;
	struct efs_block_header *hdr = NULL;
	struct fs_file_t f;

	/* Validate the input parameters */
	if (abs_path == NULL) {
		ret = -EINVAL;
	}

	/* Remove any extra slashes in the path */
	if (ret == 0) {
		ret = fsu_simplify_path(abs_path, simple_path);
		if (ret < 0) {
			LOG_ERR("efs_get_file_size: Invalid input path: %s", log_strdup(abs_path));
		} else {
			ret = 0;
		}
	}

	/* Get the encrypted size */
	if (ret == 0) {
		file_size = fsu_get_file_size_abs(simple_path);
		if (file_size < 0) {
			LOG_ERR("efs_get_file_size: Could not read flash file size: %d", file_size);
			ret = file_size;
		}
	}

	/* If the file is empty, shortcut this entire function */
	if (file_size == 0) {
		return 0;
	}

	/* Get the number of file blocks */
	num_blocks = file_size / EFS_FILE_BLOCK_SIZE;

	/* Make sure that the file is a mulitple of the encrypted block size */
	if (ret == 0) {
		if ((file_size % EFS_FILE_BLOCK_SIZE) != 0) {
			LOG_ERR("efs_get_file_size: File is not multiple of block size (%d)",
				file_size);
			ret = -EINVAL;
		}
	}

	/* Open the file */
	fs_file_t_init(&f);
	if (ret == 0) {
		ret = fs_open(&f, simple_path, FS_O_READ);
		if (ret < 0) {
			LOG_ERR("efs_get_file_size: fs_open failed %d", ret);
		}
	}

	/* Allocate memory for the file block */
	file_block = (uint8_t *)k_malloc(EFS_FILE_BLOCK_SIZE);
	if (file_block == NULL) {
		LOG_ERR("efs_get_file_size: Could not allocate memory for the file block");
		ret = -ENOMEM;
	}

	/* Allocate memory for the output block */
	user_block = (uint8_t *)k_malloc(EFS_USER_BLOCK_SIZE);
	if (user_block == NULL) {
		LOG_ERR("efs_get_file_size: Could not allocate memory for the user block");
		ret = -ENOMEM;
	}

	/* Seek to the start of the last encrypted block */
	block_offset = num_blocks - 1;
	ret = fs_seek(&f, block_offset * EFS_FILE_BLOCK_SIZE, FS_SEEK_SET);
	if (ret < 0) {
		LOG_ERR("efs_get_file_size: seek failed to block %d: %d", block_offset, ret);
	}

	/* Read the encrypted block */
	if (ret == 0) {
		ret = fs_read(&f, file_block, EFS_FILE_BLOCK_SIZE);
		if (ret < 0) {
			LOG_ERR("efs_get_file_size: read failed for block %d: %d", block_offset,
				ret);
		} else if (ret != EFS_FILE_BLOCK_SIZE) {
			LOG_ERR("efs_get_file_size: Read only returned %d bytes", ret);
			ret = -EIO;
		} else {
			/* Good read */
			ret = 0;
		}
	}

	/* Decrypt the block */
	if (ret == 0) {
		hdr = (struct efs_block_header *)file_block;
		ret = decrypt_block(simple_path, block_offset, hdr,
				    file_block + EFS_FILE_BLOCK_ENC_OFFSET,
				    EFS_FILE_BLOCK_SIZE - EFS_FILE_BLOCK_ENC_OFFSET, user_block,
				    EFS_USER_BLOCK_SIZE);
		if (ret < 0) {
			LOG_ERR("efs_get_file_size: decrypt failed for block %d", block_offset);
		}
	}

	/* Close the file */
	ret2 = fs_close(&f);
	if (ret2 < 0) {
		LOG_ERR("efs_get_file_size: Could not close file: %d", ret2);
		if (ret == 0) {
			ret = ret2;
		}
	}

	/* Compute the file size */
	if (ret == 0) {
		ret = ((num_blocks - 1) * EFS_USER_BLOCK_SIZE) + hdr->auth_data.block_size;
	}

	/* Free any memory that we allocated */
	if (file_block != NULL) {
		memset(file_block, 0, EFS_FILE_BLOCK_SIZE);
		k_free(file_block);
	}
	if (user_block != NULL) {
		memset(user_block, 0, EFS_USER_BLOCK_SIZE);
		k_free(user_block);
	}

	return ret;
}

int efs_sha256(uint8_t hash[FSU_HASH_SIZE], const char *abs_path, size_t size)
{
	char simple_path[FSU_MAX_ABS_PATH_SIZE + 1];
	int block_offset = 0;
	int this_size;
	ssize_t file_size;
	uint8_t *file_block = NULL;
	uint8_t *user_block = NULL;
	struct efs_block_header *hdr;
	struct fs_file_t f;
	int ret = 0;
	int ret2;
	psa_hash_operation_t operation = PSA_HASH_OPERATION_INIT;
	psa_status_t psa_ret;
	size_t hash_out_len = 0;

	/* Start the hash operation */
	psa_ret = psa_hash_setup(&operation, PSA_ALG_SHA_256);
	if (psa_ret != PSA_SUCCESS) {
		LOG_ERR("efs_sha256: Failed to setup hash operation: %d", psa_ret);
		ret = psa_ret;
	}

	/* Validate the input parameters */
	if (ret == 0) {
		if (abs_path == NULL || hash == NULL || size == 0) {
			LOG_ERR("efs_sha256: Invalid input parameters");
			ret = -EINVAL;
		}
	}

	/* Remove any extra slashes in the path */
	if (ret == 0) {
		ret = fsu_simplify_path(abs_path, simple_path);
		if (ret < 0) {
			LOG_ERR("efs_sha256: Invalid input path: %s", log_strdup(abs_path));
		} else {
			ret = 0;
		}
	}

	/* Get the size of the encrypted file */
	if (ret == 0) {
		file_size = fsu_get_file_size_abs(simple_path);
		if (file_size < 0) {
			LOG_ERR("efs_sha256: Could not read flash file size: %d", file_size);
			ret = file_size;
		}
	}

	/* Make sure that the file is a mulitple of the encrypted block size */
	if (ret == 0) {
		if ((file_size % EFS_FILE_BLOCK_SIZE) != 0) {
			LOG_ERR("efs_sha256: File is not multiple of block size (%d)", file_size);
			ret = -EINVAL;
		}
	}

	/* Open the file */
	fs_file_t_init(&f);
	if (ret == 0) {
		ret = fs_open(&f, simple_path, FS_O_READ);
		if (ret < 0) {
			LOG_ERR("efs_sha256: fs_open failed %d", ret);
		}
	}

	/* Allocate memory for the file block */
	file_block = (uint8_t *)k_malloc(EFS_FILE_BLOCK_SIZE);
	if (file_block == NULL) {
		LOG_ERR("efs_sha256: Could not allocate memory for the file block");
		ret = -ENOMEM;
	}

	/* Allocate memory for the output block */
	user_block = (uint8_t *)k_malloc(EFS_USER_BLOCK_SIZE);
	if (user_block == NULL) {
		LOG_ERR("efs_sha256: Could not allocate memory for the user block");
		ret = -ENOMEM;
	}

	/* Read the block(s) of the file */
	while (ret == 0 && size > 0) {
		/* Read the encrypted block */
		ret = fs_read(&f, file_block, EFS_FILE_BLOCK_SIZE);
		if (ret < 0) {
			LOG_ERR("efs_sha256: read failed for block %d", block_offset);
		} else if (ret != EFS_FILE_BLOCK_SIZE) {
			LOG_ERR("efs_sha256: Read only returned %d bytes", ret);
			ret = -EIO;
		} else {
			/* Good read */
			ret = 0;
		}

		/* Decrypt the block */
		if (ret == 0) {
			hdr = (struct efs_block_header *)file_block;
			ret = decrypt_block(simple_path, block_offset, hdr,
					    file_block + EFS_FILE_BLOCK_ENC_OFFSET,
					    EFS_FILE_BLOCK_SIZE - EFS_FILE_BLOCK_ENC_OFFSET,
					    user_block, EFS_USER_BLOCK_SIZE);
			if (ret < 0) {
				LOG_ERR("efs_sha256: decrypt failed for block %d", block_offset);
			}
		}

		/* Copy the decrypted data to the user's buffer */
		if (ret == 0) {
			/* Limit the data size to what is in this block */
			this_size = size;
			if (this_size > hdr->auth_data.block_size) {
				this_size = hdr->auth_data.block_size;
			}

			/* Update the hash with this data */
			psa_ret = psa_hash_update(&operation, user_block, this_size);
			if (psa_ret != PSA_SUCCESS) {
				LOG_ERR("efa_sha256: Hash update failed: %d", psa_ret);
				ret = -EFAULT;
			}

			/* Update counters */
			size -= this_size;
		}

		/* Move to the next block */
		if (ret == 0 && size > 0) {
			/* Don't try the next block if the current block is not full */
			if (hdr->auth_data.block_size != EFS_USER_BLOCK_SIZE) {
				break;
			} else {
				/* Try the next block */
				block_offset++;
			}
		}
	}

	/* Close the file */
	ret2 = fs_close(&f);
	if (ret2 < 0) {
		LOG_ERR("efs_sha256: Could not close file: %d", ret2);
		if (ret >= 0) {
			ret = ret2;
		}
	}

	/* Free any memory that we allocated */
	if (file_block != NULL) {
		memset(file_block, 0, EFS_FILE_BLOCK_SIZE);
		k_free(file_block);
	}
	if (user_block != NULL) {
		memset(user_block, 0, EFS_USER_BLOCK_SIZE);
		k_free(user_block);
	}

	/* Finish the hash operation */
	if (ret == 0) {
		psa_ret = psa_hash_finish(&operation, hash, FSU_HASH_SIZE, &hash_out_len);
		if (psa_ret != PSA_SUCCESS) {
			LOG_ERR("efs_sha256: Hash finish failed: %d", psa_ret);
			ret = psa_ret;
		} else if (hash_out_len != FSU_HASH_SIZE) {
			LOG_ERR("efs_sha256: Hash output not expected length (%d)", hash_out_len);
			ret = -EFAULT;
		}
	} else {
		psa_ret = psa_hash_abort(&operation);
		if (psa_ret != PSA_SUCCESS) {
			LOG_ERR("efa_sha256: Hash abort failed: %d", psa_ret);
		}
	}

	/* Return any error */
	return ret;
}

/**************************************************************************************************/
/* Local Function Definitions                                                                     */
/**************************************************************************************************/
static int file_name_hash_gen(const char *abs_path, uint8_t *hash, uint8_t hash_len)
{
	int ret = 0;
	psa_status_t s;
	size_t hash_len_out;

	/* Make sure that the hash buffer is the right size */
	if (hash_len != PSA_HASH_LENGTH(FSE_FILE_NAME_HASH_ALG)) {
		ret = -EINVAL;
	}

	/* Compute the hash */
	if (ret == 0) {
		s = psa_hash_compute(FSE_FILE_NAME_HASH_ALG, abs_path, strlen(abs_path), hash,
				     hash_len, &hash_len_out);
		if (s == PSA_SUCCESS && hash_len != hash_len_out) {
			ret = -EFAULT;
		} else {
			ret = s;
		}
	}

	return ret;
}

static int file_name_hash_comp(const char *abs_path, uint8_t *hash, uint8_t hash_len)
{
	int ret = 0;
	psa_status_t s;

	/* Make sure that the hash buffer is the right size */
	if (hash_len != PSA_HASH_LENGTH(FSE_FILE_NAME_HASH_ALG)) {
		ret = -EINVAL;
	}

	/* Compute the hash */
	if (ret == 0) {
		s = psa_hash_compare(FSE_FILE_NAME_HASH_ALG, abs_path, strlen(abs_path), hash,
				     hash_len);
		ret = s;
	}

	return ret;
}

static int encrypt_block(struct efs_block_header *hdr, uint8_t *user_data, ssize_t user_data_len,
			 uint8_t *out)
{
	uint32_t enc_size = 0;
	int ret = 0;

	/* Validate the input */
	if (hdr == NULL || user_data == NULL || user_data_len == 0 ||
	    user_data_len > EFS_USER_BLOCK_SIZE) {
		ret = -EINVAL;
	}

	/* Generate a new IV for this block */
	if (ret == 0) {
		ret = lcz_hw_key_generate_iv(hdr->iv, LCZ_HW_KEY_IV_LEN);
		if (ret != 0) {
			LOG_ERR("Block encrypt failed to generate IV: %d", ret);
		}
	}

	/* Encrypt the block */
	if (ret == 0) {
		ret = lcz_hw_key_encrypt_data(hdr->iv, sizeof(hdr->iv),
					      (const uint8_t *)&(hdr->auth_data),
					      sizeof(hdr->auth_data), user_data, user_data_len, out,
					      EFS_USER_BLOCK_SIZE + LCZ_HW_KEY_MAC_LEN, &enc_size);
		if (ret != 0) {
			LOG_ERR("Block encrypt failed: %d", ret);
		} else if (enc_size != (LCZ_HW_KEY_MAC_LEN + user_data_len)) {
			LOG_ERR("Block encrypt output not expected length (%d != %d)", enc_size,
				LCZ_HW_KEY_MAC_LEN + user_data_len);
			ret = -EFAULT;
		}
	}

	return ret;
}

static int decrypt_block(const char *abs_path, int block_number, struct efs_block_header *hdr,
			 uint8_t *in_data, int in_data_len, uint8_t *out_data, int out_data_len)
{
	int ret;
	uint32_t out_data_len_ret = 0;

	/* Decrypt and authenticate the block */
	ret = lcz_hw_key_decrypt_data(hdr->iv, sizeof(hdr->iv), (const uint8_t *)&(hdr->auth_data),
				      sizeof(hdr->auth_data), in_data, in_data_len, out_data,
				      out_data_len, &out_data_len_ret);

	/* Compare provided data with authenticated header data */
	if (ret != 0) {
		LOG_ERR("decrypt_block: Decrypt failed: %d", ret);
	} else if (out_data_len_ret != out_data_len) {
		LOG_ERR("decrypt_block: Decrypt output not expected length (%d != %d)",
			out_data_len_ret, out_data_len);
		ret = -EFAULT;
	} else if (hdr->auth_data.block_number != block_number ||
		   file_name_hash_comp(abs_path, hdr->auth_data.file_name_hash,
				       sizeof(hdr->auth_data.file_name_hash)) != 0) {
		LOG_ERR("decrypt_block: Decryption succeeded, but metadata is incorrect");
		memset(out_data, 0, out_data_len);
		ret = -EINVAL;
	}

	return ret;
}

/**************************************************************************************************/
/* SYS_INIT                                                                                       */
/**************************************************************************************************/
SYS_INIT(lcz_enc_fs_init, APPLICATION, CONFIG_FSU_ENCRYPTED_FILES_INIT_PRIORITY);
static int lcz_enc_fs_init(const struct device *device)
{
	ARG_UNUSED(device);
	int r = 0;

	if (strlen(CONFIG_FSU_ENCRYPTED_FILE_PATH)) {
		r = fs_mkdir(CONFIG_FSU_ENCRYPTED_FILE_PATH);
		if (r != 0 && r != -EEXIST) {
			LOG_ERR("lcz_fs_mgmt_init: mkdir failed: %d", r);
		}
	}

	return r;
}
