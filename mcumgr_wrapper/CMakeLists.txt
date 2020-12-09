#
# Laird Connectivity Zephyr Components
#
# Copyright (c) 2020 Laird Connectivity
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(include)

zephyr_sources_ifdef(CONFIG_LCZ_MCUMGR_WRAPPER source/mcumgr_wrapper.c)
zephyr_sources_ifdef(CONFIG_LCZ_MCUMGR_SIMPLE_BLUETOOTH source/simple_bluetooth.c)

if(CONFIG_MCUMGR)
zephyr_link_libraries(MCUMGR INTERFACE
  zephyr_interface
  )
endif()

if(CONFIG_FILE_SYSTEM_LITTLEFS)
zephyr_link_libraries(LITTLEFS INTERFACE
  zephyr_interface
)
endif()
