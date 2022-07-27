#
# Copyright (c) 2022 Laird Connectivity
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_LCZ_MCUMGR_CMD_FS_MGMT)
zephyr_include_directories(lcz_fs_mgmt/include)

zephyr_sources(
    lcz_fs_mgmt/src/lcz_fs_mgmt.c
    lcz_fs_mgmt/src/lcz_zephyr_fs_mgmt.c
)
endif()
