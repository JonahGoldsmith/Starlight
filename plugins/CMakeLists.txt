cmake_minimum_required(VERSION 3.2)

project(sl_default_plugins)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins)

include_directories(../plugins)

include_directories(../base)

add_subdirectory(os_window)