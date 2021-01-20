#
# Laird Connectivity Zephyr Components
#
# Copyright (c) 2020 Laird Connectivity
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(include)

zephyr_sources_ifdef(CONFIG_LCZ_QRTC source/lcz_qrtc.c)
zephyr_sources_ifdef(CONFIG_LCZ_LED source/lcz_led.c)
zephyr_sources_ifdef(CONFIG_LCZ_DNS source/lcz_dns.c)
zephyr_sources_ifdef(CONFIG_LCZ_POWER source/laird_power.c)
zephyr_sources_ifdef(CONFIG_LCZ_HWREV source/laird_connectivity_hwrev.c)
zephyr_sources_ifdef(CONFIG_LCZ_NFC source/laird_connectivity_nfc.c)
zephyr_sources_ifdef(CONFIG_LCZ_CUSTOM_FATAL_ERROR_HANDLER
    source/custom_fatal_error_handler.c)
zephyr_sources_ifdef(CONFIG_LCZ_SOCK source/lcz_sock.c)
zephyr_sources_ifdef(CONFIG_LCZ_RESET_ON_EXIT source/lcz_reset_on_exit.c)
zephyr_sources_ifdef(CONFIG_LCZ_PARAMS source/lcz_params.c)
zephyr_sources_ifdef(CONFIG_LCZ_PARAMS_SHELL source/lcz_params_shell.c)
zephyr_sources_ifdef(CONFIG_LCZ_PWM_LED source/lcz_pwm_led.c)
zephyr_sources_ifdef(CONFIG_LCZ_NO_INIT_RAM_VAR source/lcz_no_init_ram_var.c)
zephyr_sources_ifdef(CONFIG_LCZ_SOFTWARE_RESET source/lcz_software_reset.c)

