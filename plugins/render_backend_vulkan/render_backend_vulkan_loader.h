// MIT License
//
// Copyright (c) 2022-2023 Jonah Goldsmith
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//

#ifndef RENDER_BACKEND_VULKAN_LOADER_H
#define RENDER_BACKEND_VULKAN_LOADER_H

#include "base/defines.h"

#define ENABLE_GRAPHICS_DEBUG 1
#define MAX_INSTANCE_EXTENSIONS 64

#if !defined(__ANDROID__)
#define ENABLE_DEBUG_UTILS_EXTENSION
#endif

#if SL_PLATFORM_WINDOWS || SL_PLATFORM_XBOXONE
#define VK_USE_PLATFORM_WIN32_KHR
#elif SL_PLATFORM_ANDROID
#ifndef VK_USE_PLATFORM_ANDROID_KHR
#define VK_USE_PLATFORM_ANDROID_KHR
#endif
#elif SL_PLATFORM_LINUX
// TODO: Separate vulkan ext from choosing xlib vs xcb
#define VK_USE_PLATFORM_XLIB_KHR    //Use Xlib or Xcb as display server, defaults to Xlib
#elif defined(__APPLE__)
#define VK_USE_PLATFORM_METAL_EXT
#endif

#define CHECK_VKRESULT(exp)                                                      \
	{                                                                            \
		VkResult vkres = (exp);                                                  \
		if (VK_SUCCESS != vkres)                                                 \
		{                                                                        \
			SL_LOG_ERROR("%s: FAILED with VkResult: %d\n", #exp, vkres); \
			SL_ASSERT(false, "See Last Error");                                                       \
		}                                                                        \
	}

#define SAFE_FREE(alloc, p_var)       \
	if (p_var)                 \
	{                          \
		sl_free(alloc, (void*)p_var); \
	}

#if defined(__cplusplus)
#define DECLARE_ZERO(type, var) type var = {};
#else
#define DECLARE_ZERO(type, var) type var = { 0 };
#endif

struct vk_gpu_settings
{
	bool render_doc_layer_enabled;
	bool dedicated_allocations;
	bool memory_req_2_ext;
	bool frag_shader_interlock_ext;
	bool draw_indirect_count;
	bool descriptor_indexing;
	bool dynamic_rendering;
	bool amd_draw_indirect_count;
	bool amd_gcn_shader_extension;
	bool ycbr_conversion_extension;
	bool buffer_device_address;
#if SL_PLATFORM_WINDOWS
	bool external_memory_ext;
	bool external_memory_win32_ext;
#endif
};

#endif//RENDER_BACKEND_VULKAN_LOADER_H
