cmake_minimum_required(VERSION 3.1)

project(sl_shader_compiler)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES main.cpp)

set(INCLUDES )

add_executable(${PROJECT_NAME} ${SOURCES} ${INCLUDES} )

if(PLATFORM_MACOS)
add_library(directx_compiler SHARED IMPORTED)
set_property(TARGET directx_compiler PROPERTY IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/third-party/dxc/lib/macos/libdxcompiler.dylib
        )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE sl_base directx_compiler)
target_include_directories(${PROJECT_NAME} PRIVATE sl_base ${CMAKE_SOURCE_DIR}/third-party/dxc/include)

if (WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE LINKS_SL_BASE)

# Set property for my_target only
set_property(TARGET ${PROJECT_NAME} PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")