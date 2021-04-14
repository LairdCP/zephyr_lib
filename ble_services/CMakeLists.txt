#
# Laird Connectivity Zephyr BLE Services
#
# Copyright (c) 2020-2021 Laird Connectivity
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(../ble_common/include)
zephyr_include_directories(../components/include)
zephyr_include_directories(../utilities/include)
zephyr_include_directories(include)

zephyr_sources_ifdef(CONFIG_BT source/vsp_definitions.c)

zephyr_sources_ifdef(CONFIG_UNIVERSAL_BOOTLOADER_SERVICE source/universal_bootloader_service.c)
zephyr_sources_ifdef(CONFIG_BLE_CELLULAR_SERVICE source/ble_cellular_service.c)
zephyr_sources_ifdef(CONFIG_BLE_POWER_SERVICE source/ble_power_service.c)
zephyr_sources_ifdef(CONFIG_BLE_ESS_SERVICE source/ble_ess_service.c)
zephyr_sources_ifdef(CONFIG_LCZ_BLE_DIS source/dis.c)
zephyr_sources_ifdef(CONFIG_FOTA_SERVICE source/fota.c)
