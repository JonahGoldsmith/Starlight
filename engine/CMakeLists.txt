cmake_minimum_required(VERSION 3.1)

project(sl_engine)

set(SOURCES main.c register_engine_apis.h register_engine_apis.c)

set(INCLUDES )

add_executable(${PROJECT_NAME} ${SOURCES} ${INCLUDES} )

target_link_libraries(${PROJECT_NAME} PRIVATE sl_base)
target_include_directories(${PROJECT_NAME} PRIVATE sl_base)

if (WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE LINKS_SL_BASE)
