#
# Laird Connectivity Zephyr Components
#
zephyr_include_directories(include)

zephyr_sources_ifdef(CONFIG_LCZ_QRTC source/qrtc.c)
zephyr_sources_ifdef(CONFIG_LCZ_LED source/laird_led.c)
zephyr_sources_ifdef(CONFIG_LCZ_DNS source/lcz_dns.c)
zephyr_sources_ifdef(CONFIG_LCZ_POWER source/laird_power.c)
zephyr_sources_ifdef(CONFIG_LCZ_HWREV source/laird_connectivity_hwrev.c)
zephyr_sources_ifdef(CONFIG_LCZ_NFC source/laird_connectivity_nfc.c)
zephyr_sources_ifdef(CONFIG_LCZ_CUSTOM_FATAL_ERROR_HANDLER
    source/custom_fatal_error_handler.c)
zephyr_sources_ifdef(CONFIG_LCZ_SOCK source/lcz_sock.c)
