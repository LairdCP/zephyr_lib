#
# Copyright (c) 2020 Laird Connectivity
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories_ifdef(CONFIG_LCZ_DRIVER include ../include/drivers)

zephyr_sources_ifdef(CONFIG_PIEZO piezo/piezo.c)
zephyr_sources_ifdef(CONFIG_VIBEMOTOR vibemotor/vibe.c)
zephyr_sources_ifdef(CONFIG_ACCELEROMETER accelerometer/accelerometer.c)
zephyr_sources_ifdef(CONFIG_LCZ_NRF_QSPI_NOR lcz_nrf_qspi_nor/lcz_nrf_qspi_nor.c)
zephyr_sources_ifdef(CONFIG_LCZ_BL5340PA bl5340pa/bl5340pa.c)

zephyr_sources_ifdef(CONFIG_MG100_LIS2DH mg100_lis2dh/mg100_lis2dh.c)
zephyr_sources_ifdef(CONFIG_MG100_LIS2DH mg100_lis2dh/mg100_lis2dh_i2c.c)
zephyr_sources_ifdef(CONFIG_MG100_LIS2DH mg100_lis2dh/mg100_lis2dh_spi.c)
zephyr_sources_ifdef(CONFIG_MG100_LIS2DH mg100_lis2dh/mg100_lis2dh_trigger.c)
