# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
add_definitions(-DBOARD_ROOT=${PROJECT_SOURCE_DIR})
add_definitions(-DNRFX_TWIS_ENABLED=1 -DNRFX_TWIS0_ENABLED=1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(badge)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)
