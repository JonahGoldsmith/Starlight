cmake_minimum_required(VERSION 3.2)

project(os_window)

if(PLATFORM_WIN32)
    add_library(${PROJECT_NAME} SHARED os_window.h os_window_win32.c)
endif()

if(PLATFORM_MACOS)
    add_library(${PROJECT_NAME} SHARED os_window.h os_window_macos.m)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(QUARTZ_LIBRARY QuartzCore)
    find_library(METALKIT_LIBRARY MetalKit)
    find_library(METAL_FRAMEWORK Metal)

    target_link_libraries(
            ${PROJECT_NAME}
            PUBLIC
            ${COCOA_LIBRARY}
            ${QUARTZ_LIBRARY}
            ${METALKIT_LIBRARY}
            ${METAL_FRAMEWORK}
    )
endif()

add_subdirectory(third-party/glfw)

target_include_directories(${PROJECT_NAME} PUBLIC glfw ${GLFW_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PUBLIC glfw ${GLFW_LIBRARIES})

# Set property for my_target only
set_property(TARGET ${PROJECT_NAME} PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")

