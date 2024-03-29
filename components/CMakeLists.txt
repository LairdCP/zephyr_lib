#
# Laird Connectivity Zephyr Components
#
# Copyright (c) 2020-2022 Laird Connectivity
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(include)

zephyr_sources_ifdef(CONFIG_LCZ_QRTC source/lcz_qrtc.c)
zephyr_sources_ifdef(CONFIG_LCZ_QRTC_SHELL source/lcz_qrtc_shell.c)
zephyr_sources_ifdef(CONFIG_MCUMGR_CMD_QRTC_MGMT source/qrtc_mgmt.c)
zephyr_sources_ifdef(CONFIG_LCZ_LED source/lcz_led.c)
zephyr_sources_ifdef(CONFIG_LCZ_DNS source/lcz_dns.c)
zephyr_sources_ifdef(CONFIG_LCZ_HWREV source/laird_connectivity_hwrev.c)
zephyr_sources_ifdef(CONFIG_LCZ_NFC source/laird_connectivity_nfc.c)
zephyr_sources_ifdef(CONFIG_LCZ_CUSTOM_FATAL_ERROR_HANDLER
    source/custom_fatal_error_handler.c)
zephyr_sources_ifdef(CONFIG_LCZ_SOCK source/lcz_sock.c)
zephyr_sources_ifdef(CONFIG_LCZ_RESET_ON_EXIT source/lcz_reset_on_exit.c)
zephyr_sources_ifdef(CONFIG_LCZ_PARAM_FILE source/lcz_param_file.c)
zephyr_sources_ifdef(CONFIG_LCZ_PARAM_FILE_SHELL source/lcz_param_file_shell.c)
zephyr_sources_ifdef(CONFIG_LCZ_PWM_LED source/lcz_pwm_led.c)
zephyr_sources_ifdef(CONFIG_LCZ_NO_INIT_RAM_VAR source/lcz_no_init_ram_var.c)
zephyr_sources_ifdef(CONFIG_LCZ_SOFTWARE_RESET source/lcz_software_reset.c)
zephyr_sources_ifdef(CONFIG_LCZ_EVENT_MANAGER source/lcz_event_manager.c
    source/lcz_event_manager_file_handler.c)
zephyr_sources_ifdef(CONFIG_MCUMGR_CMD_EVENT_LOG_MGMT source/event_log_mgmt.c)
zephyr_sources_ifdef(CONFIG_LCZ_BRACKET source/lcz_bracket.c)
zephyr_sources_ifdef(CONFIG_LCZ_APPROTECT source/lcz_approtect.c)
zephyr_sources_ifdef(CONFIG_LCZ_RPMSG source/lcz_rpmsg.c)
zephyr_sources_ifdef(CONFIG_LCZ_WDT source/lcz_watchdog.c)
zephyr_sources_ifdef(CONFIG_MCUMGR_CMD_SHELL_LOG_MGMT source/shell_log_mgmt.c)
zephyr_sources_ifdef(CONFIG_DUMMY_SMP source/dummy_smp.c)
zephyr_sources_ifdef(CONFIG_LCZ_RAMDISK source/lcz_ramdisk.c)
