#
# Laird Connectivity Zephyr BLE Common Components
#
# Copyright (c) 2021 Laird Connectivity
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories_ifdef(CONFIG_LCZ_BT ../utilities/include)
zephyr_include_directories(include)

zephyr_sources_ifdef(CONFIG_LCZ_BT source/lcz_bluetooth.c)
zephyr_sources_ifdef(CONFIG_LCZ_AD_FIND source/ad_find.c)
zephyr_sources_ifdef(CONFIG_LCZ_BT_SCAN source/lcz_bt_scan.c)
zephyr_sources_ifdef(CONFIG_LCZ_SENSOR_ADV_FORMAT source/lcz_sensor_adv_format.c)
zephyr_sources_ifdef(CONFIG_LCZ_SENSOR_EVENT source/lcz_sensor_event.c)
