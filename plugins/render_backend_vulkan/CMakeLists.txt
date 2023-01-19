cmake_minimum_required(VERSION 3.2)

project(render_backend_vulkan)

add_library(${PROJECT_NAME} SHARED render_backend_vulkan.h render_backend_vulkan.cpp)