cmake_minimum_required(VERSION 3.2)

project(render_backend_metal)

if(PLATFORM_MACOS)
    add_library(${PROJECT_NAME} SHARED render_backend_metal.h render_backend_metal.mm metal_availability_macros.h)
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

    # Set property for my_target only
    set_property(TARGET ${PROJECT_NAME} PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")


endif()