# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lcz_watchdog_basic_api)

FILE(GLOB app_sources src/main.c src/test*.c)
target_sources(app PRIVATE ${app_sources})
