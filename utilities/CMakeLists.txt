#
# Laird Connectivity Zephyr Utilities
#
# Copyright (c) 2020 Laird Connectivity
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(include)

zephyr_sources(source/hexcode.c)
zephyr_sources(source/string_util.c)
zephyr_sources(source/print_json.c)
zephyr_sources_ifdef(CONFIG_FILE_SYSTEM_UTILITIES source/file_system_utilities.c)
zephyr_sources_ifdef(CONFIG_FSU_ENCRYPTED_FILES source/encrypted_file_storage.c)
zephyr_sources_ifdef(CONFIG_FSU_SHELL source/fsu_shell.c)
zephyr_sources_ifdef(CONFIG_ERRNO_STR source/errno_str.c)
zephyr_sources_ifdef(CONFIG_LCZ_NRF_RESET_REASON source/lcz_nrf_reset_reason.c)
zephyr_sources_ifdef(CONFIG_LCZ_SNPRINTK source/lcz_snprintk.c)

if(CONFIG_FILE_SYSTEM_LITTLEFS)
zephyr_link_libraries(LITTLEFS INTERFACE
  zephyr_interface
)
endif()

zephyr_link_libraries_ifdef(CONFIG_FSU_HASH mbedTLS)