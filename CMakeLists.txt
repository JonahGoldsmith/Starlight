cmake_minimum_required(VERSION 3.1)
project(Starlight)

#Generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#Detect Clang-CL
if(CMAKE_C_COMPILER_ID MATCHES "Clang" AND MSVC)
    set(CLANG_CL 1 CACHE INTERNAL BOOL "")
    message(STATUS "using clang-cl compiler")
endif()

#If MSVC is enabled use Multi-Threaded Compilation
if (MSVC)
    option(MSVC_STATIC_RUNTIME "Link with MSVC static runtime. Only sets for 'Release' build" OFF)
    option(MSVC_MULTITHREADED_COMPILE "Multi-threaded compilation in MSVC." ON)
endif()

#If Clang is being used disable clang build profiler
if (CMAKE_C_COMPILER_ID MATCHES "Clang")
    option(CLANG_ENABLE_PROFILER "Enable clang build profiler ('-ftime-trace') flag" OFF)
endif()

if (NOT MSVC)
    set(CMAKE_CXX_STANDARD 14)
endif()

# output directories for binary files
set(bin_dir bin)

if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${bin_dir})
endif()
if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${bin_dir})
endif()

function(sl_remove_compile_options dst_var compile_flags flags)
    separate_arguments(flags)
    foreach(flag ${flags})
        string(REPLACE "${flag}" "" compile_flags "${compile_flags}")
    endforeach()
    set(${dst_var} ${compile_flags} PARENT_SCOPE)
endfunction()

function(sl_add_plugin proj_name source_files)
    if (BUNDLE)
        add_library(${proj_name} STATIC ${source_files})
    else()
        add_library(${proj_name} SHARED ${source_files})
    endif()

    set_target_properties(${proj_name} PROPERTIES FOLDER plugins)
endfunction()

add_definitions(-D__STDC_LIMIT_MACROS)
add_definitions(-D__STDC_FORMAT_MACROS)
add_definitions(-D__STDC_CONSTANT_MACROS)
if(WIN32)
    add_definitions(-D_ITERATOR_DEBUG_LEVEL=0)
    add_definitions(-D_SECURE_SCL=0)
    add_definitions(-D_HAS_EXCEPTIONS=0)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS=0)
endif()


macro(sl_set_compile_flags_current_dir)
    if (MSVC)
        sl_remove_compile_options(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" "/EHsc /GR")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
        if (MSVC_MULTITHREADED_COMPILE AND NOT CMAKE_MAKE_PROGRAM MATCHES "ninja.exe")
            add_compile_options(/MP)    # multiprocessor compilation
        endif()

        if (MSVC_STATIC_RUNTIME)
            sl_remove_compile_options(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}" "/MD")
            sl_remove_compile_options(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}" "/MD")
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
            set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")

            sl_remove_compile_options(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}" "/MDd")
            sl_remove_compile_options(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}" "/MDd")
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
            set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
        endif()

        if (${MSVC_VERSION} GREATER_EQUAL 1928)
            message(STATUS "MSVC compiler supports C17 standard")
            # add_compile_options(/std:c17)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /std:c17")
        endif()

        sl_remove_compile_options(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}" "/Ob0")
        sl_remove_compile_options(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}" "/Ob0")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Ob1")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Ob1")
    else()
        if (NOT WIN32)
            add_compile_options(-fPIC)
        endif()
        add_compile_options(-Wno-deprecated-declarations)
        add_compile_options("$<$<CONFIG:Debug>:-D_DEBUG>")
        add_compile_options("$<$<CONFIG:Release>:-DNDEBUG>")

        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")
        sl_remove_compile_options(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" "-fexceptions -frtti")

        if (CMAKE_C_COMPILER_ID MATCHES "GNU")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-format-truncation")
        endif()
    endif()

    if (CLANG_ENABLE_PROFILER)
        string(FIND ${CMAKE_C_COMPILER_VERSION} "." major_ver_end)
        if (${major_ver_end} GREATER 0)
            string(SUBSTRING ${CMAKE_C_COMPILER_VERSION} 0 ${major_ver_end} clang_major_version)
            if (${clang_major_version} GREATER_EQUAL 9)
                # https://aras-p.info/blog/2019/01/16/time-trace-timeline-flame-chart-profiler-for-Clang/
                # https://github.com/aras-p/ClangBuildAnalyzer
                message(STATUS "Enabling clang built-in profiler: -ftime-trace")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_RELEASE} -ftime-trace")
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_RELEASE} -ftime-trace")
            else()
                message(WARNING "Current clang version does not support built-in profiler")
            endif()
        endif()
    endif()
endmacro()

sl_set_compile_flags_current_dir()

# clang-cl complains about returning structs from c-extern return type, which is valid, but we don't need them now
if (CLANG_CL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-return-type-c-linkage")
endif()

# include directory is for all projects
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/)

option(BUILD_DOC "Build documentation" ON)

find_package(Doxygen)
if (DOXYGEN_FOUND)
    # set input and output files
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Doxyfile)

    # request to configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")

    # note the option ALL which allows to build the docs together with the application
    add_custom_target( doc_doxygen ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM )
else (DOXYGEN_FOUND)
    message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)


set(PLATFORM_WIN32 FALSE CACHE INTERNAL "")
set(PLATFORM_UNIVERSAL_WINDOWS FALSE CACHE INTERNAL "")
set(PLATFORM_ANDROID FALSE CACHE INTERNAL "")
set(PLATFORM_LINUX FALSE CACHE INTERNAL "")
set(PLATFORM_MACOS FALSE CACHE INTERNAL "")
set(PLATFORM_IOS FALSE CACHE INTERNAL "")
set(PLATFORM_EMSCRIPTEN FALSE CACHE INTERNAL "")

if(WIN32)
    if(${CMAKE_SYSTEM_NAME} STREQUAL "WindowsStore")
        set(PLATFORM_UNIVERSAL_WINDOWS TRUE CACHE INTERNAL "Target platform: Windows Store")
        message("Target platform: Universal Windows. SDK Version: " ${CMAKE_SYSTEM_VERSION})
    else()
        set(PLATFORM_WIN32 TRUE CACHE INTERNAL "Target platform: Win32") #WIN32 is a variable, so we cannot use string "WIN32"
        message("Target platform: Win32. SDK Version: " ${CMAKE_SYSTEM_VERSION})
    endif()
else()
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
        set(PLATFORM_ANDROID TRUE CACHE INTERNAL "Target platform: Android")
        message("Target platform: Android")
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        set(PLATFORM_LINUX TRUE CACHE INTERNAL "Target platform: Linux")
        message("Target Platform: Linux")
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        if(IOS)
            set(PLATFORM_IOS TRUE CACHE INTERNAL "Target platform: iOS")
            message("Target Platform: iOS")
        else()
            set(PLATFORM_MACOS TRUE CACHE INTERNAL "Target platform: MacOS")
            message("Target Platform: MacOS")
        endif()
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
        set(PLATFORM_IOS TRUE CACHE INTERNAL "Target platform: iOS")
        message("Target Platform: iOS")
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "tvOS")
        set(PLATFORM_TVOS TRUE CACHE INTERNAL "Target platform: tvOS")
        message("Target Platform: tvOS")
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
        set(PLATFORM_EMSCRIPTEN TRUE CACHE INTERNAL "Target platform: Emscripten")
        message("Target Platform: Emscripten")
    else()
        message(FATAL_ERROR "Unsupported platform")
    endif()
endif(WIN32)

add_subdirectory(base)
add_subdirectory(engine)
add_subdirectory(plugins)