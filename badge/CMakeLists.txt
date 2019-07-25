# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
set(BOARD dcfurs_bt832a)
set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
add_definitions(-DNRFX_TWIS_ENABLED=1 -DNRFX_TWIS0_ENABLED=1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(badge)

FILE(GLOB app_sources *.c)
target_sources(app PRIVATE ${app_sources})

zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)
