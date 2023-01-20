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

#include "render_backend_vulkan_loader.h"
#include "registry/plugin_util.inl"
#include "render_backend/render_backend.h"
#include "render_backend_vulkan.h"
#include "base/memory/allocator.h"
#include "base/logging/logger.h"
#include "base/util/assertions.inl"
#include "base/data_structures/array.inl"

static sl_logger_api* sl_logger_api;
static sl_allocator* backend_allocator;

#define VOLK_IMPLEMENTATION
#include "volk.h"
#ifdef __APPLE__
#include <vulkan/vulkan_metal.h>
#endif


SL_PRAGMA_DIAGNOSTIC_PUSH_CLANG_()
SL_PRAGMA_DIAGNOSTIC_IGNORED_CLANG("-Wnullability-completeness")
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
SL_PRAGMA_DIAGNOSTIC_POP()

#define ENABLE_GRAPHICS_DEBUG 1
#define MAX_INSTANCE_EXTENSIONS 64


#pragma region vulkan_util

const char* vk_wanted_instance_extensions[] =
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
		VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XCB_KHR)
		VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
		VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_GGP)
		VK_GGP_STREAM_DESCRIPTOR_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_VI_NN)
		VK_NN_VI_SURFACE_EXTENSION_NAME,
#elif defined(__APPLE__)
		VK_EXT_METAL_SURFACE_EXTENSION_NAME,
		VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#endif
// Debug utils not supported on all devices yet
#if ENABLE_GRAPHICS_DEBUG
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
		//VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
		// To legally use HDR formats
		VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME,
/************************************************************************/
// Multi GPU Extensions
/************************************************************************/
#if VK_KHR_device_group_creation
		//VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME,
#endif
#ifndef NX64
		/************************************************************************/
		// Property querying extensions
		/************************************************************************/
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
#endif
};

#define CHECK_VKRESULT(exp)                                                      \
	{                                                                            \
		VkResult vkres = (exp);                                                  \
		if (VK_SUCCESS != vkres)                                                 \
		{                                                                        \
			SL_LOG_ERROR("%s: FAILED with VkResult: %d\n", #exp, vkres); \
			SL_ASSERT(false, "See Last Error");                                                       \
		}                                                                        \
	}

static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT message_type,
	const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	void* user_data) {

	switch(message_severity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		SL_LOG_ERROR("%s\n", callback_data->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		SL_LOG_INFO("%s\n", callback_data->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		SL_LOG_DEBUG("%s\n", callback_data->pMessage);
		break;
#ifdef ENABLE_GRAPHICS_DEBUG
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		SL_LOG_DEBUG("%s\n", callback_data->pMessage);
#endif
	}

	return VK_FALSE;
}

static void* VKAPI_PTR gVkAllocation(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
	return backend_allocator->realloc(backend_allocator, 0, size, alignment, SL_FUNCTION, __FILE__, __LINE__);
}

static void* VKAPI_PTR
gVkReallocation(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
	return backend_allocator->realloc(backend_allocator, pOriginal, size, alignment, SL_FUNCTION, __FILE__, __LINE__);
}

static void VKAPI_PTR gVkFree(void* pUserData, void* pMemory) { backend_allocator->realloc(backend_allocator, pMemory, 0, 0, SL_FUNCTION, __FILE__, __LINE__); }

static void VKAPI_PTR
gVkInternalAllocation(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
}

static void VKAPI_PTR
gVkInternalFree(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
}

VkAllocationCallbacks vk_allocation_callbacks =
	{
		// pUserData
		NULL,
		// pfnAllocation
		gVkAllocation,
		// pfnReallocation
		gVkReallocation,
		// pfnFree
		gVkFree,
		// pfnInternalAllocation
		gVkInternalAllocation,
		// pfnInternalFree
		gVkInternalFree
};

#pragma endregion

#pragma region vulkan_objects

struct vulkan_backend
{
	sl_allocator* allocator;
	VkInstance instance;
	VkDevice device;
	VkPhysicalDevice physical_device;
#if ENABLE_GRAPHICS_DEBUG
	VkDebugUtilsMessengerEXT debug_messenger;
#endif
};

#pragma endregion

#pragma region vulkan_swapchain
sl_swapchain* vulkan_create_swapchain(struct sl_render_backend* backend, sl_swapchain_desc* desc)
{
	SL_NOT_IMPLEMENTED();
}

void vulkan_destroy_swapchain(struct sl_render_backend* backend, sl_swapchain* swapchain)
{

	SL_NOT_IMPLEMENTED();

}

void vulkan_present_swapchain(struct sl_render_backend* backend, sl_swapchain* swapchain)
{
	SL_NOT_IMPLEMENTED();

}
#pragma endregion



bool vulkan_create_backend(sl_render_backend* backend, sl_allocator* allocator)
{
	vulkan_backend* vk = (vulkan_backend*)sl_alloc(allocator, sizeof(vulkan_backend));
	/*
	 * Initialization
	 */

	backend_allocator = allocator;

	const char** instanceLayers = (const char**)alloca((2) * sizeof(char*));
	uint32_t     instanceLayerCount = 0;

#if ENABLE_GRAPHICS_DEBUG
	// this turns on all validation layers
	instanceLayers[instanceLayerCount++] = "VK_LAYER_KHRONOS_validation";
#endif

	// this turns on render doc layer for gpu capture
#ifdef ENABLE_RENDER_DOC
	instanceLayers[instanceLayerCount++] = "VK_LAYER_RENDERDOC_Capture";
#endif

	CHECK_VKRESULT(volkInitialize());

	// These are the extensions that we have loaded
		const char* instanceExtensionCache[MAX_INSTANCE_EXTENSIONS] = {};

	uint32_t layerCount = 0;
	uint32_t extCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, NULL);
	vkEnumerateInstanceExtensionProperties(NULL, &extCount, NULL);

	VkLayerProperties* layers = (VkLayerProperties*)alloca(sizeof(VkLayerProperties) * layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, layers);

	VkExtensionProperties* exts = (VkExtensionProperties*)alloca(sizeof(VkExtensionProperties) * extCount);
	vkEnumerateInstanceExtensionProperties(NULL, &extCount, exts);

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext = NULL;
	//TODO CONFIGURABLE APP NAME
	app_info.pApplicationName = "Starlight";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "Starlight";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_2;

#if ENABLE_GRAPHICS_DEBUG
	for(int i = 0; i < instanceLayerCount; i++)
	{
		for(int j = 0; j < layerCount; j++) {
			if (strcmp(instanceLayers[i], layers[j].layerName) == 0)
			{
				SL_LOG_INFO("Found Layer %s\n", instanceLayers[i]);
			}
		}
	}
#endif

	uint32_t extension_count = 0;
	const uint32_t initialCount = sizeof(vk_wanted_instance_extensions) / sizeof(vk_wanted_instance_extensions[0]);
	char* found_extensions[initialCount];
	for(int i = 0; i < initialCount; i++)
	{
		for(int j = 0; j < extCount; j++)
		{
			if (strcmp(vk_wanted_instance_extensions[i], exts[j].extensionName) == 0)
			{
				extension_count+=1;
				SL_LOG_INFO("Found Inst Ext %s\n", vk_wanted_instance_extensions[i]);
			}
		}
	}

#if VK_HEADER_VERSION >= 108
	VkValidationFeaturesEXT      validationFeaturesExt = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
	VkValidationFeatureEnableEXT enabledValidationFeatures[] = {
		VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
	};

#if ENABLE_GRAPHICS_DEBUG
		validationFeaturesExt.enabledValidationFeatureCount = 1;
		validationFeaturesExt.pEnabledValidationFeatures = enabledValidationFeatures;
#endif
#endif


		VkInstanceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#if VK_HEADER_VERSION >= 108
		create_info.pNext = &validationFeaturesExt;
#endif
#ifdef __APPLE__
		create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#else
		create_info.flags =0;
#endif
		create_info.pApplicationInfo = &app_info;
		create_info.enabledLayerCount = instanceLayerCount;
		create_info.ppEnabledLayerNames = instanceLayers;
		create_info.enabledExtensionCount = extension_count;
		create_info.ppEnabledExtensionNames = vk_wanted_instance_extensions;

		CHECK_VKRESULT(vkCreateInstance(&create_info, &vk_allocation_callbacks, &vk->instance));

		volkLoadInstance(vk->instance);

#if ENABLE_GRAPHICS_DEBUG
		VkDebugUtilsMessengerCreateInfoEXT db_create_info = {};
		db_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		db_create_info.pfnUserCallback = vk_debug_callback;
		db_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		db_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		db_create_info.flags = 0;
		db_create_info.pUserData = NULL;
		CHECK_VKRESULT(vkCreateDebugUtilsMessengerEXT(
						   vk->instance, &db_create_info, &vk_allocation_callbacks, &vk->debug_messenger));
		//TODO: DEBUG REPORT INSTEAD OF DEBUG UTIL?
#endif
		//TODO: Split these up into different files for clarity
		/*
		 * Picking Physical Device
		 */
		VkPhysicalDevice physicalDevices[16];
		uint32_t physicalDeviceCount = sizeof(physicalDevices) / sizeof(physicalDevices[0]);
		CHECK_VKRESULT(vkEnumeratePhysicalDevices(vk->instance, &physicalDeviceCount, physicalDevices));

		for (uint32_t i = 0; i < physicalDeviceCount; ++i)
		{
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(physicalDevices[i], &props);

			if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				SL_LOG_INFO("Picking discrete GPU: %s\n", props.deviceName);
				vk->physical_device = physicalDevices[i];
				break;
			}

			if (physicalDeviceCount > 0)
			{
				VkPhysicalDeviceProperties props;
				vkGetPhysicalDeviceProperties(physicalDevices[0], &props);

				SL_LOG_INFO("Picking fallback GPU: %s\n", props.deviceName);
				vk->physical_device = physicalDevices[0];
			}

			if(!vk->physical_device) {
				SL_LOG_ERROR("Failed to find a suitable GPU!\n");
				return false;
			}

		}

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(vk->physical_device, &queueFamilyCount, nullptr);

		VkQueueFamilyProperties queueFamilies[queueFamilyCount];
		vkGetPhysicalDeviceQueueFamilyProperties(vk->physical_device, &queueFamilyCount, queueFamilies);

		//End picking Device
		uint32_t index = 0;
		for(int i = 0; i < queueFamilyCount; i++)
		{
			if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				index = i;
				break;
			}
		}

		//Create Logical Device
		float queuePriorities[] = { 1.0f };

		VkDeviceQueueCreateInfo queueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queueInfo.queueFamilyIndex = index;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = queuePriorities;

		const char* device_extensions[] =
			{
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
				"VK_KHR_portability_subset",
#endif
			};

		VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		createInfo.queueCreateInfoCount = 1;
		createInfo.pQueueCreateInfos = &queueInfo;

		createInfo.ppEnabledExtensionNames = device_extensions;
		createInfo.enabledExtensionCount = sizeof(device_extensions) / sizeof(device_extensions[0]);

		CHECK_VKRESULT(vkCreateDevice(vk->physical_device, &createInfo, &vk_allocation_callbacks, &vk->device));


		/*
		 * Set the function pointers
		 */
	backend->create_swapchain = vulkan_create_swapchain;
	backend->destroy_swapchain = vulkan_destroy_swapchain;
	backend->present_swapchain = vulkan_present_swapchain;

	vk->allocator = allocator;

	backend->inst = vk;
	return true;
}

void vulkan_destroy_backend(sl_render_backend* backend)
{
	vulkan_backend* vk = (vulkan_backend*)backend->inst;

	vkDestroyDevice(vk->device, &vk_allocation_callbacks);

#if ENABLE_GRAPHICS_DEBUG
	vkDestroyDebugUtilsMessengerEXT(vk->instance, vk->debug_messenger, &vk_allocation_callbacks);
#endif

	vkDestroyInstance(vk->instance, &vk_allocation_callbacks);
	sl_free(vk->allocator, vk);
}

struct sl_render_backend_vulkan_api vulkan_api = {
	vulkan_create_backend,
	vulkan_destroy_backend
};

PLUGIN_EXPORT int sl_load_plugin(struct sl_api_registry* reg, struct cr_plugin *ctx, enum cr_op operation)
{

	switch(operation) {
		
	case CR_LOAD:
		reg->set(RENDER_BACKEND_VULKAN_API, &vulkan_api, sizeof(struct sl_render_backend_vulkan_api));
		sl_logger_api = (struct sl_logger_api*)reg->get(SL_LOGGER_API);
		return 0;
		break;
	case CR_UNLOAD:
		return 0;
		break;
	case CR_STEP:
		break;
	case CR_CLOSE:
		break;
	}

	return 0;

}