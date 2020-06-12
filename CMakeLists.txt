# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories_ifdef(CONFIG_COMPONENT_LIB include include/drivers)

zephyr_sources_ifdef(CONFIG_PIEZO drivers/piezo/piezo.c)
zephyr_sources_ifdef(CONFIG_VIBEMOTOR drivers/vibemotor/vibe.c)