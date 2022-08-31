#
# Copyright (c) 2022 Laird Connectivity LLC
#
# SPDX-License-Identifier: LicenseRef-LairdConnectivity-Clause
#
if(CONFIG_LCZ_KVP)
    zephyr_include_directories(include)
    zephyr_sources(source/lcz_kvp.c)
endif()

if (CONFIG_LCZ_KVP_LOG_HEXDUMP)
  message(WARNING "LCZ KVP hexdump should not be enabled for production builds")
endif()