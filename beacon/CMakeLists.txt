# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
set(BOARD dcfurs_fox)
set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(beacon)

target_sources(app PRIVATE main.c)

