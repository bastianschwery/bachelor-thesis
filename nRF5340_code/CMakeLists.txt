#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.13.1)
set(BOARD nrf5340dk_nrf5340_cpuappns)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(PerCen)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.cpp src/DeviceManager.h src/DeviceManager.cpp src/Data.h src/Data.cpp src/DataService.h src/DataService.cpp src/BatteryManager.h src/BatteryManager.c
)
# NORDIC SDK APP END
zephyr_library_include_directories(.)