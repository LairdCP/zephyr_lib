#
# Laird Connectivity Zephyr Memfault Components
#
# Copyright (c) 2021 Laird Connectivity
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(include)

zephyr_sources_ifdef(CONFIG_LCZ_MEMFAULT_HTTP_TRANSPORT source/lcz_memfault_http.c)
zephyr_sources_ifdef(CONFIG_LCZ_MEMFAULT_MQTT_TRANSPORT source/lcz_memfault_mqtt.c)

# Override the warning in the Nordic file (memfault_integration)
# with a link to Laird Connectivity's instructions.
if(CONFIG_LCZ_MEMFAULT AND NOT CONFIG_MEMFAULT_NCS_PROJECT_KEY)
	message(FATAL_ERROR
		"
		Memfault Project Key not configured.
	Please visit https://goto.memfault.com/create-key/pinnacle-100 or
	Disable Memfault with CONFIG_LCZ_MEMFAULT=n
		")
endif()
