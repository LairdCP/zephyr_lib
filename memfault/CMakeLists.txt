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
