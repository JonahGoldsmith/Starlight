cmake_minimum_required(VERSION 3.2)

project(os_window)

add_library(${PROJECT_NAME} SHARED os_window.h os_window_macos.m)