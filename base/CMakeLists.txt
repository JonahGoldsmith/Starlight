cmake_minimum_required(VERSION 3.1)

project(sl_base)


set(BASE
        defines.h
        platform.h
        )

set(DATA_STRUCTURES
        data_structures/array.inl
        data_structures/hash.inl
        data_structures/mpmc_queue.h)

set(LOGGING
        logging/logger.h
        logging/logger.c)

set(MEMORY
        memory/allocator.h
        memory/allocator.c
        memory/mem_tracker.h
        memory/mem_tracker.c)

set(OS
        os/os_win32.c
        os/os_macos.c
        os/os.h)

set(THREAD
        thread/mutex.inl
        thread/atomics.inl
        thread/spinlock.inl
        thread/job_system.c
        thread/job_system.h)

set(UTIL
        util/assertions.inl
        util/error.h
        util/sprintf.c
        util/sprintf.h
        util/path_util.inl)

set(REGISTRY
        registry/plugin_system.h
        registry/plugin_system.cpp
        registry/api_registry.c
        registry/api_registry.h)

set(SOURCES
        ${UTIL} ${OS} ${DATA_STRUCTURES} ${THREAD} ${MEMORY} ${BASE} ${LOGGING} ${REGISTRY})

#Third Party Header Files or Sources Built into the Static Lib
set(THIRD_PARTY_SOURCES
        third-party/c89atomic/c89atomic.h
        third-party/stb/stb_sprintf.h
        third-party/minicoro/minicoro.h)

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${THIRD_PARTY_SOURCES})

# compat files include dir
if (WIN32)
    set(COMPAT_DIR third-party/compat/msvc)
elseif(APPLE)
    if (IOS)
        set(COMPAT_DIR third-party/compat/ios)
    else()
        set(COMPAT_DIR third-party/compat/osx)
    endif()
endif()

target_include_directories(${PROJECT_NAME}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/
        PUBLIC ${COMPAT_DIR})

# Set property for my_target only
set_property(TARGET ${PROJECT_NAME} PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")