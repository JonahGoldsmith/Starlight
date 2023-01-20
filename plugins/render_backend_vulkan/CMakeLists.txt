cmake_minimum_required(VERSION 3.2)

project(render_backend_vulkan)

add_library(${PROJECT_NAME} SHARED
        render_backend_vulkan.h render_backend_vulkan.cpp
        render_backend_vulkan_loader.cpp render_backend_vulkan_loader.h)

add_subdirectory(third-party/volk)
add_subdirectory(third-party/VulkanMemoryAllocator)

target_link_libraries(${PROJECT_NAME} PUBLIC volk_headers vulkan_memory_allocator)

target_include_directories(${PROJECT_NAME} PUBLIC volk_headers vulkan_memory_allocator)