# Search Algorithm 
# Copyright 2026 Daynlight
# Licensed under the GNU General.
# See LICENSE file for details.



cmake_minimum_required(VERSION 3.14)

project(search LANGUAGES CXX)
find_package(fmt CONFIG REQUIRED)
find_package(indicators CONFIG REQUIRED)

set(src
  search/main.cpp
)

add_executable(search ${src})
target_link_libraries(search PRIVATE
  fmt::fmt
  indicators::indicators
)

