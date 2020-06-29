#
# Laird Connectivity Zephyr BLE Common Components
#

zephyr_include_directories(../utilities/include)
zephyr_include_directories(include)

zephyr_sources_ifdef(CONFIG_BT source/laird_bluetooth.c)

zephyr_sources_ifdef(CONFIG_LCZ_AD_FIND source/ad_find.c)
zephyr_sources_ifdef(CONFIG_LCZ_BT_SCAN source/bt_scan.c)
