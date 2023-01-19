cmake_minimum_required(VERSION 3.2)

project(render_backend)

#This cmake lists file is basically only here to have auto-completion work better!
#In VSCode and CLion
add_library(${PROJECT_NAME} INTERFACE render_backend.h)

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/)