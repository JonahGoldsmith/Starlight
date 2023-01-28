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

//Availability Macros
#if defined(VK_KHR_ray_tracing_pipeline) && defined(VK_KHR_acceleration_structure) && !defined(__APPLE__)
#define VK_RAYTRACING_AVAILABLE
#endif

#define VK_DEBUG_LOG_EXTENSIONS 0

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
#ifdef ENABLE_DEBUG_UTILS_EXTENSION
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#else
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
		VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
		// To legally use HDR formats
		VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME,
/************************************************************************/
// VR Extensions //TODO: only on Windows and Linux
/************************************************************************/
//VK_KHR_DISPLAY_EXTENSION_NAME,
//VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME,
/************************************************************************/
// Multi GPU Extensions
/************************************************************************/
#if VK_KHR_device_group_creation
		VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME,
#endif
#ifndef NX64
		/************************************************************************/
		// Property querying extensions
		/************************************************************************/
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
/************************************************************************/
/************************************************************************/
#endif
};

const char* vk_wanted_device_extensions[] =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MAINTENANCE1_EXTENSION_NAME,
		VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
		VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME,
		VK_EXT_SHADER_SUBGROUP_VOTE_EXTENSION_NAME,
		VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
#ifdef __APPLE__
		"VK_KHR_portability_subset",
#endif
#ifdef USE_EXTERNAL_MEMORY_EXTENSIONS
		VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
		VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
		VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
		VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME,
		VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME,
#endif
#endif
// Debug marker extension in case debug utils is not supported
#ifndef ENABLE_DEBUG_UTILS_EXTENSION
		VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
#endif
#if defined(VK_USE_PLATFORM_GGP)
		VK_GGP_FRAME_TOKEN_EXTENSION_NAME,
#endif

#if VK_KHR_draw_indirect_count
		VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
#endif
// Fragment shader interlock extension to be used for ROV type functionality in Vulkan
#if VK_EXT_fragment_shader_interlock
		VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME,
#endif
/************************************************************************/
// NVIDIA Specific Extensions
/************************************************************************/
#ifdef USE_NV_EXTENSIONS
		VK_NVX_DEVICE_GENERATED_COMMANDS_EXTENSION_NAME,
#endif
		/************************************************************************/
		// AMD Specific Extensions
		/************************************************************************/
		VK_AMD_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
		VK_AMD_SHADER_BALLOT_EXTENSION_NAME,
		VK_AMD_GCN_SHADER_EXTENSION_NAME,
/************************************************************************/
// Multi GPU Extensions
/************************************************************************/
#if VK_KHR_device_group
		VK_KHR_DEVICE_GROUP_EXTENSION_NAME,
#endif
		/************************************************************************/
		// Bindless & None Uniform access Extensions
		/************************************************************************/
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
#if VK_KHR_maintenance3 // descriptor indexing depends on this
		VK_KHR_MAINTENANCE3_EXTENSION_NAME,
#endif
/************************************************************************/
// Raytracing
/************************************************************************/
#ifdef VK_RAYTRACING_AVAILABLE
		VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,

		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_SPIRV_1_4_EXTENSION_NAME,
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,

		VK_KHR_RAY_QUERY_EXTENSION_NAME,
#endif
/************************************************************************/
// YCbCr format support
/************************************************************************/
#if VK_KHR_bind_memory2
		// Requirement for VK_KHR_sampler_ycbcr_conversion
		VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
#endif
#if VK_KHR_sampler_ycbcr_conversion
		VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,
		VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
		VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
#endif
#if VK_KHR_image_format_list
		VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME ,
#endif
/************************************************************************/
// Multiview support
/************************************************************************/
#ifdef QUEST_VR
		VK_KHR_MULTIVIEW_EXTENSION_NAME,
#endif
/************************************************************************/
// Nsight Aftermath
/************************************************************************/
#ifdef ENABLE_NSIGHT_AFTERMATH
		VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME,
		VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME,
#endif
		/************************************************************************/
		/************************************************************************/
};

#if ENABLE_GRAPHICS_DEBUG
#ifdef ENABLE_DEBUG_UTILS_EXTENSION
// Debug callback for Vulkan layers
static VkBool32 VKAPI_PTR internal_debug_report_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	const char* pLayerPrefix = pCallbackData->pMessageIdName;
	const char* pMessage = pCallbackData->pMessage;
	int32_t     messageCode = pCallbackData->messageIdNumber;
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		SL_LOG_INFO("[%s] : %s (%i)\n", pLayerPrefix, pMessage, messageCode);
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		SL_LOG_DEBUG("[%s] : %s (%i)\n", pLayerPrefix, pMessage, messageCode);
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		SL_LOG_ERROR("[%s] : %s (%i)\n", pLayerPrefix, pMessage, messageCode);
		SL_ASSERT(false, "See previous message!^");
	}

	return VK_FALSE;
}
#else
// gAssertOnVkValidationError is used to work around a bug in the ovr mobile sdk.
// There is a fence creation struct that is not initialized in the sdk.
bool gAssertOnVkValidationError = true;

static VKAPI_ATTR VkBool32 VKAPI_CALL internal_debug_report_callback(
	VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode,
	const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
	if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
	{
		SL_LOG_INFO("[%s] : %s (%i)\n", pLayerPrefix, pMessage, messageCode);
	}
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		SL_LOG_DEBUG("[%s] : %s (%i)\n", pLayerPrefix, pMessage, messageCode);
	}
	else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
	{
		SL_LOG_DEBUG("[%s] : %s (%i)\n", pLayerPrefix, pMessage, messageCode);
	}
	else if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		SL_LOG_ERROR("[%s] : %s (%i)\n", pLayerPrefix, pMessage, messageCode);
		if (gAssertOnVkValidationError)
		{
			SL_ASSERT(false, "See last message!^");
		}
	}

	return VK_FALSE;
}
#endif
#endif

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

#define ADD_TO_NEXT_CHAIN(condition, next)        \
	if ((condition))                              \
	{                                             \
		base->pNext = (VkBaseOutStructure*)&next; \
		base = (VkBaseOutStructure*)base->pNext;  \
	}

#pragma endregion

#pragma region vulkan_objects

enum queue_types
{
	QUEUE_GRAPHICS,
	QUEUE_PRESENT,
	QUEUE_TRANSFER,
	QUEUE_COMPUTE,
	QUEUE_MAX
};

struct vulkan_backend
{
	sl_allocator* allocator;
	VkInstance instance;
	VkDevice device;
	VkPhysicalDevice active_physical_device;
	vk_gpu_settings active_gpu_settings;
	VmaAllocator vma_allocator;
	//TODO: MULTI GPU AND BETTER QUEUE ORGANIZATION
	uint32_t queue_indices[QUEUE_MAX];
	VkQueue graphics_queue;
	VkQueue present_queue;
	VkQueue transfer_queue;
	VkQueue compute_queue;
#ifdef ENABLE_DEBUG_UTILS_EXTENSION
	VkDebugUtilsMessengerEXT debug_messenger;
#else
	VkDebugReportCallbackEXT debug_report;
#endif
	bool device_group_creation_extension;
	bool debug_utils_extension;
	bool enable_gpu_validation;
};

struct vulkan_swapchain
{
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	sl_swapchain_desc desc;
};

#pragma endregion


#pragma region vulkan_swapchain
sl_swapchain* vulkan_create_swapchain(struct sl_render_backend* backend, sl_swapchain_desc* desc)
{
	vulkan_swapchain vk_swapchain = sl_alloc(backend_allocator, sizeof(vulkan_swapchain));
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

static void vulkan_create_instance(vulkan_backend* vk, uint32_t instance_layer_count, const char** instance_layers)
{
	// These are the extensions that we have loaded
	const char*instance_extension_cache[MAX_INSTANCE_EXTENSIONS] = {};

	uint32_t layer_count = 0;
	uint32_t ext_count = 0;
	vkEnumerateInstanceLayerProperties(&layer_count, NULL);
	vkEnumerateInstanceExtensionProperties(NULL, &ext_count, NULL);

	VkLayerProperties* layers = (VkLayerProperties*)alloca(sizeof(VkLayerProperties) * layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, layers);

	VkExtensionProperties* exts = (VkExtensionProperties*)alloca(sizeof(VkExtensionProperties) * ext_count);
	vkEnumerateInstanceExtensionProperties(NULL, &ext_count, exts);

#if VK_DEBUG_LOG_EXTENSIONS
	for (uint32_t i = 0; i < layer_count; ++i)
	{
		SL_LOG_INFO("vkinstance-layer: %s\n", layers[i].layerName);
	}

	for (uint32_t i = 0; i < ext_count; ++i)
	{
		SL_LOG_INFO("vkinstance-ext: %s\n", exts[i].extensionName);
	}
#endif

	DECLARE_ZERO(VkApplicationInfo, app_info);
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext = NULL;
	app_info.pApplicationName = "Starlight";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "StarlightEngine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_2;

	const char** layer_temp = NULL;
	sl_array_set_capacity(vk->allocator, layer_temp, instance_layer_count);

	// check to see if the layers are present
	for (uint32_t i = 0; i < instance_layer_count; ++i)
	{
		bool layer_found = false;
		for (uint32_t j = 0; j < layer_count; ++j)
		{
			if (strcmp(instance_layers[i], layers[j].layerName) == 0)
			{
				layer_found = true;
				sl_array_push(vk->allocator, layer_temp, instance_layers[i]);
				break;
			}
		}
		if (layer_found == false)
		{
			SL_LOG_ERROR("vkinstance-layer-missing: %s\n", instance_layers[i]);
		}
	}

	uint32_t                   extension_count = 0;
	const uint32_t             initial_count = sizeof(vk_wanted_instance_extensions) / sizeof(vk_wanted_instance_extensions[0]);
	const char** wanted_inst_extensions = NULL;
	sl_array_resize(vk->allocator, wanted_inst_extensions, initial_count);
	for (uint32_t i = 0; i < initial_count; ++i)
	{
		wanted_inst_extensions[i] = vk_wanted_instance_extensions[i];
	}
	const uint32_t wanted_extension_count = sl_array_size(wanted_inst_extensions);

	// Layer extensions
	for (uint32_t i = 0; i < sl_array_size(layer_temp); ++i)
	{
		const char* layer_name = layer_temp[i];
		uint32_t    count = 0;
		vkEnumerateInstanceExtensionProperties(layer_name, &count, NULL);
		VkExtensionProperties* properties = count ? (VkExtensionProperties*)sl_alloc(vk->allocator, count * sizeof(*properties)) : NULL;
		SL_ASSERT(properties != NULL || count == 0, "Failed to allocate memory!");
		vkEnumerateInstanceExtensionProperties(layer_name, &count, properties);
		for (uint32_t j = 0; j < count; ++j)
		{
			for (uint32_t k = 0; k < wanted_extension_count; ++k)
			{
				if (strcmp(wanted_inst_extensions[k], properties[j].extensionName) == 0)    //-V522
				{
					if (strcmp(wanted_inst_extensions[k], VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME) == 0)
						vk->device_group_creation_extension = true;
#ifdef ENABLE_DEBUG_UTILS_EXTENSION
					if (strcmp(wanted_inst_extensions[k], VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
						vk->debug_utils_extension = true;
#endif
					instance_extension_cache[extension_count++] = wanted_inst_extensions[k];
					// clear wanted extension so we dont load it more then once
					wanted_inst_extensions[k] = "";
					break;
				}
			}
		}
		SAFE_FREE(vk->allocator, (void*)properties);
	}

	// Standalone extensions
	{
		const char* layer_name = NULL;
		uint32_t    count = 0;
		vkEnumerateInstanceExtensionProperties(layer_name, &count, NULL);
		if (count > 0)
		{
			VkExtensionProperties* properties = (VkExtensionProperties*)sl_alloc(vk->allocator, count * sizeof(*properties));
			SL_ASSERT(properties != NULL, "Failed to allocate memory!");
			vkEnumerateInstanceExtensionProperties(layer_name, &count, properties);
			for (uint32_t j = 0; j < count; ++j)
			{
				for (uint32_t k = 0; k < wanted_extension_count; ++k)
				{
					if (strcmp(wanted_inst_extensions[k], properties[j].extensionName) == 0)
					{
						instance_extension_cache[extension_count++] = wanted_inst_extensions[k];

						if (strcmp(wanted_inst_extensions[k], VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME) == 0)
							vk->device_group_creation_extension = true;
#ifdef ENABLE_DEBUG_UTILS_EXTENSION
						if (strcmp(wanted_inst_extensions[k], VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
							vk->debug_utils_extension = true;
#endif
						break;
					}
				}
			}
			SAFE_FREE(vk->allocator, (void*)properties);
		}
	}

#if VK_HEADER_VERSION >= 108
	VkValidationFeaturesEXT      validationFeaturesExt = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
	VkValidationFeatureEnableEXT enabledValidationFeatures[] = {
		VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
	};

	if (vk->enable_gpu_validation)
	{
		validationFeaturesExt.enabledValidationFeatureCount = 1;
		validationFeaturesExt.pEnabledValidationFeatures = enabledValidationFeatures;
	}
#endif
	{
		// Add more extensions here
		DECLARE_ZERO(VkInstanceCreateInfo, create_info);
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#if VK_HEADER_VERSION >= 108
		create_info.pNext = &validationFeaturesExt;
#endif
#ifdef __APPLE__
		create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#else
		create_info.flags = 0;
#endif
		create_info.pApplicationInfo = &app_info;
		create_info.enabledLayerCount = sl_array_size(layer_temp);
		create_info.ppEnabledLayerNames = layer_temp;
		create_info.enabledExtensionCount = extension_count;
		create_info.ppEnabledExtensionNames = instance_extension_cache;

		SL_LOG_INFO("Creating VkInstance with %i enabled instance layers:\n", sl_array_size(layer_temp));
		for (int i = 0; i < sl_array_size(layer_temp); i++) {
			SL_LOG_INFO("Layer %i: %s\n", i, layer_temp[i]);
		}
		SL_LOG_INFO("And with %i: enabled extensions:\n", extension_count);
		for(int i = 0; i < extension_count; i++)
		{
			SL_LOG_INFO("Extension %i: %s\n", i+1, instance_extension_cache[i]);
		}


		CHECK_VKRESULT(vkCreateInstance(&create_info, &vk_allocation_callbacks, &(vk->instance)));
		sl_array_free(vk->allocator, layer_temp);
		sl_array_free(vk->allocator, wanted_inst_extensions);
	}
#if defined(NX64)
	loadExtensionsNX(vk->instance);
#else
	// Load Vulkan instance functions
	volkLoadInstance(vk->instance);
#endif

	// Debug
#if ENABLE_GRAPHICS_DEBUG
{
#ifdef ENABLE_DEBUG_UTILS_EXTENSION
	if (vk->debug_utils_extension)
	{
		VkDebugUtilsMessengerCreateInfoEXT create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		create_info.pfnUserCallback = internal_debug_report_callback;
		create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		create_info.flags = 0;
		create_info.pUserData = NULL;
		VkResult res = vkCreateDebugUtilsMessengerEXT(
			vk->instance, &create_info, &vk_allocation_callbacks, &(vk->debug_messenger));
		if (VK_SUCCESS != res)
		{
			SL_LOG_ERROR("vkCreateDebugUtilsMessengerEXT failed - disabling Vulkan debug callbacks\n");
		}
	}
#else
#if defined(__ANDROID__)
	if (vkCreateDebugReportCallbackEXT)
#endif
	{
		DECLARE_ZERO(VkDebugReportCallbackCreateInfoEXT, create_info);
		create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
		create_info.pNext = NULL;
		create_info.pfnCallback = internal_debug_report_callback;
		create_info.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT |
#if defined(NX64) || defined(__ANDROID__)
			VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |    // Performance warnings are not very vaild on desktop
#endif
			VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT /* | VK_DEBUG_REPORT_INFORMATION_BIT_EXT*/;
		VkResult res = vkCreateDebugReportCallbackEXT(
			vk->instance, &create_info, &vk_allocation_callbacks, &(vk->debug_report));
		if (VK_SUCCESS != res)
		{
			SL_LOG_ERROR("vkCreateDebugReportCallbackEXT failed - disabling Vulkan debug callbacks\n");
		}
	}
#endif
}
#endif

}

static bool vulkan_init_common(vulkan_backend* vk)
{
	//TODO: dispatch Tables
#if defined(VK_USE_DISPATCH_TABLES)
	VkResult vkRes = volkInitializeWithDispatchTables(vk);
	if (vkRes != VK_SUCCESS)
	{
		SL_LOG_ERROR("Failed to initialize Vulkan\n");
		nvapiExit();
		agsExit();
		return false;
	}
#else
	const char** instance_layers = (const char**)alloca((2) * sizeof(char*));
	uint32_t instance_layer_count = 0;

#if defined(ENABLE_GRAPHICS_DEBUG)
	// this turns on all validation layers
	instance_layers[instance_layer_count++] = "VK_LAYER_KHRONOS_validation";
#endif

	// this turns on render doc layer for gpu capture
#ifdef ENABLE_RENDER_DOC
	instanceLayers[instanceLayerCount++] = "VK_LAYER_RENDERDOC_Capture";
#endif

#if !defined(NX64)
	VkResult vkRes = volkInitialize();
	if (vkRes != VK_SUCCESS)
	{
		SL_LOG_ERROR("Failed to initialize Vulkan\n");
		return false;
	}
#endif

	vulkan_create_instance(vk, instance_layer_count, instance_layers);
#endif

	return true;
}

bool vulkan_add_device(vulkan_backend* vk)
{

	// These are the extensions that we have loaded
	const char* device_extension_cache[64] = {};

	uint32_t gpu_count = 0;
	CHECK_VKRESULT(vkEnumeratePhysicalDevices(vk->instance, &gpu_count, NULL));

	if(gpu_count < 1) {
		SL_ASSERT(gpu_count, "Failed to Find a Vulkan Device");
		return false;
	}

	VkPhysicalDevice gpus[gpu_count];
	VkPhysicalDeviceProperties2 gpu_properties[gpu_count];
	VkPhysicalDeviceMemoryProperties gpu_mem_properties[gpu_count];
	VkPhysicalDeviceFeatures2KHR gpu_features[gpu_count];
	CHECK_VKRESULT(vkEnumeratePhysicalDevices(vk->instance, &gpu_count, gpus));

	//TODO: MULTI-GPU

	/************************************************************************/
	// Select discrete gpus first
	// If we have multiple discrete gpus prefer with bigger VRAM size
	// To find VRAM in Vulkan, loop through all the heaps and find if the
	// heap has the DEVICE_LOCAL_BIT flag set
	/************************************************************************/

	for (uint32_t i = 0; i < gpu_count; ++i)
	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(gpus[i], &props);

		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			SL_LOG_INFO("Picking discrete GPU: %s\n", props.deviceName);
			vk->active_physical_device = gpus[i];
			break;
		}

		if (gpu_count > 0)
		{
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(gpus[0], &props);

			SL_LOG_INFO("Picking fallback GPU: %s\n", props.deviceName);
			vk->active_physical_device = gpus[0];
		}

	}

	if(!vk->active_physical_device) {
		SL_LOG_ERROR("Failed to find a suitable GPU!\n");
		return false;
	}

	uint32_t layer_count = 0;
	uint32_t ext_count = 0;
	vkEnumerateDeviceLayerProperties(vk->active_physical_device, &layer_count, NULL);
	vkEnumerateDeviceExtensionProperties(vk->active_physical_device, NULL, &ext_count, NULL);

	VkLayerProperties layers[layer_count];
	vkEnumerateDeviceLayerProperties(vk->active_physical_device, &layer_count, layers);

	VkExtensionProperties exts[ext_count];
	vkEnumerateDeviceExtensionProperties(vk->active_physical_device, NULL, &ext_count, exts);

#if VK_DEBUG_LOG_EXTENSIONS
	for (uint32_t i = 0; i < layer_count; ++i)
	{
		SL_LOG_INFO("vkdevice-layer: %s\n", layers[i].layerName);
	}

	for (uint32_t i = 0; i < ext_count; ++i)
	{
		SL_LOG_INFO("vkdevice-ext: %s\n", exts[i].extensionName);
	}
#endif

	for (uint32_t i = 0; i < layer_count; ++i)
	{
		if (strcmp(layers[i].layerName, "VK_LAYER_RENDERDOC_Capture") == 0)
		{
			vk->active_gpu_settings.render_doc_layer_enabled = true;
		}
	}

	uint32_t extension_count = 0;
	// Standalone extensions
	{
		const char*		layer_name = NULL;
		uint32_t initial_count = sizeof(vk_wanted_device_extensions) / sizeof(vk_wanted_device_extensions[0]);
		const char** wanted_device_extensions = NULL;
		sl_array_resize(vk->allocator, wanted_device_extensions, initial_count);
		for (uint32_t i = 0; i < initial_count; ++i)
		{
			wanted_device_extensions[i] = vk_wanted_device_extensions[i];
		}
		const uint32_t wanted_extension_count = sl_array_size(wanted_device_extensions);
		uint32_t       count = 0;
		vkEnumerateDeviceExtensionProperties(vk->active_physical_device, layer_name, &count, NULL);
		if (count > 0)
		{
			VkExtensionProperties* properties = (VkExtensionProperties*)sl_alloc(vk->allocator, count * sizeof(*properties));
			SL_ASSERT(properties != NULL, "Failed to allocate memory!");
			vkEnumerateDeviceExtensionProperties(vk->active_physical_device, layer_name, &count, properties);
			for (uint32_t j = 0; j < count; ++j)
			{
				for (uint32_t k = 0; k < wanted_extension_count; ++k)
				{
					if (strcmp(wanted_device_extensions[k], properties[j].extensionName) == 0)
					{
						device_extension_cache[extension_count++] = wanted_device_extensions[k];

						if(strcmp(wanted_device_extensions[k], VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME) == 0)
							vk->active_gpu_settings.dynamic_rendering = true;

#ifndef ENABLE_DEBUG_UTILS_EXTENSION
						//TODO: DEBUG MARKER
						if (strcmp(wantedDeviceExtensions[k], VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0)
							vk->mVulkan.mDebugMarkerSupport = true;
#endif
						if (strcmp(wanted_device_extensions[k], VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME) == 0)
							vk->active_gpu_settings.dedicated_allocations = true;
						if (strcmp(wanted_device_extensions[k], VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME) == 0)
							vk->active_gpu_settings.memory_req_2_ext = true;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
						if (strcmp(wantedDeviceExtensions[k], VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME) == 0)
							externalMemoryExtension = true;
						if (strcmp(wantedDeviceExtensions[k], VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME) == 0)
							externalMemoryWin32Extension = true;
#endif
#if VK_KHR_draw_indirect_count
						if (strcmp(wanted_device_extensions[k], VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME) == 0)
							vk->active_gpu_settings.draw_indirect_count = true;
#endif
						if (strcmp(wanted_device_extensions[k], VK_AMD_DRAW_INDIRECT_COUNT_EXTENSION_NAME) == 0)
							vk->active_gpu_settings.amd_draw_indirect_count = true;
						if (strcmp(wanted_device_extensions[k], VK_AMD_GCN_SHADER_EXTENSION_NAME) == 0)
							vk->active_gpu_settings.amd_gcn_shader_extension = true;
#if VK_EXT_descriptor_indexing
						if (strcmp(wanted_device_extensions[k], VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME) == 0)
							vk->active_gpu_settings.descriptor_indexing = true;
#endif
						if(strcmp(wanted_device_extensions[k], VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) == 0)
							vk->active_gpu_settings.buffer_device_address = true;
#ifdef VK_RAYTRACING_AVAILABLE
						//TODO: raytracing !
						// KHRONOS VULKAN RAY TRACING
						uint32_t khrRaytracingSupported = 1;

						if (strcmp(wantedDeviceExtensions[k], VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME) == 0)
							vk->mVulkan.mShaderFloatControlsExtension = 1;
						khrRaytracingSupported &= vk->mVulkan.mShaderFloatControlsExtension;

						if (strcmp(wantedDeviceExtensions[k], VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) == 0)
							vk->mVulkan.mBufferDeviceAddressExtension = 1;
						khrRaytracingSupported &= vk->mVulkan.mBufferDeviceAddressExtension;

						if (strcmp(wantedDeviceExtensions[k], VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME) == 0)
							vk->mVulkan.mDeferredHostOperationsExtension = 1;
						khrRaytracingSupported &= vk->mVulkan.mDeferredHostOperationsExtension;

						if (strcmp(wantedDeviceExtensions[k], VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) == 0)
							vk->mVulkan.mKHRAccelerationStructureExtension = 1;
						khrRaytracingSupported &= vk->mVulkan.mKHRAccelerationStructureExtension;

						if (strcmp(wantedDeviceExtensions[k], VK_KHR_SPIRV_1_4_EXTENSION_NAME) == 0)
							vk->mVulkan.mKHRSpirv14Extension = 1;
						khrRaytracingSupported &= vk->mVulkan.mKHRSpirv14Extension;

						if (strcmp(wantedDeviceExtensions[k], VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) == 0)
							vk->mVulkan.mKHRRayTracingPipelineExtension = 1;
						khrRaytracingSupported &= vk->mVulkan.mKHRRayTracingPipelineExtension;

						if (khrRaytracingSupported)
							vk->mVulkan.mRaytracingSupported = 1;

						if (strcmp(wantedDeviceExtensions[k], VK_KHR_RAY_QUERY_EXTENSION_NAME) == 0)
							vk->mVulkan.mKHRRayQueryExtension = 1;
#endif
#if VK_KHR_sampler_ycbcr_conversion
						if (strcmp(wanted_device_extensions[k], VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME) == 0)
						{
							vk->active_gpu_settings.ycbr_conversion_extension = true;
						}
#endif
#if VK_EXT_fragment_shader_interlock
						if (strcmp(wanted_device_extensions[k], VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME) == 0)
						{
							vk->active_gpu_settings.frag_shader_interlock_ext = true;
						}
#endif
#if defined(QUEST_VR)
						if (strcmp(wantedDeviceExtensions[k], VK_KHR_MULTIVIEW_EXTENSION_NAME) == 0)
						{
							vk->mVulkan.mMultiviewExtension = true;
						}
#endif
#ifdef ENABLE_NSIGHT_AFTERMATH
						if (strcmp(wantedDeviceExtensions[k], VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME) == 0)
						{
							vk->mDiagnosticCheckPointsSupport = true;
						}
						if (strcmp(wantedDeviceExtensions[k], VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME) == 0)
						{
							vk->mDiagnosticsConfigSupport = true;
						}
#endif
						break;
					}
				}
			}
			SAFE_FREE(vk->allocator, properties);
		}
		sl_array_free(vk->allocator, wanted_device_extensions);
	}

#if !defined(VK_USE_DISPATCH_TABLES)
	VkPhysicalDeviceFeatures2KHR gpuFeatures2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR };
	VkBaseOutStructure* base = (VkBaseOutStructure*)&gpuFeatures2; //-V1027

	// Add more extensions here
#if VK_EXT_fragment_shader_interlock
	VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT fragmentShaderInterlockFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT };
	ADD_TO_NEXT_CHAIN(vk->active_gpu_settings.frag_shader_interlock_ext, fragmentShaderInterlockFeatures);
#endif
#if VK_EXT_descriptor_indexing
	VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT };
	ADD_TO_NEXT_CHAIN(vk->active_gpu_settings.descriptor_indexing, descriptorIndexingFeatures);
#endif
#if VK_KHR_sampler_ycbcr_conversion
	VkPhysicalDeviceSamplerYcbcrConversionFeatures ycbcrFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES };
	ADD_TO_NEXT_CHAIN(vk->active_gpu_settings.ycbr_conversion_extension, ycbcrFeatures);
#endif
#if defined(QUEST_VR)
	VkPhysicalDeviceMultiviewFeatures multiviewFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES };
	ADD_TO_NEXT_CHAIN(vk->mVulkan.mMultiviewExtension, multiviewFeatures);
#endif


#if VK_KHR_buffer_device_address
	VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddressFeatures = {};
	enabledBufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	enabledBufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
	ADD_TO_NEXT_CHAIN(vk->active_gpu_settings.buffer_device_address, enabledBufferDeviceAddressFeatures);
#endif
#if !defined(__APPLE__)
#if VK_KHR_ray_tracing_pipeline
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures = {};
	enabledRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
	ADD_TO_NEXT_CHAIN(vk->mVulkan.mKHRRayTracingPipelineExtension, enabledRayTracingPipelineFeatures);
#endif
#if VK_KHR_acceleration_structure
	VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures = {};
	enabledAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
	ADD_TO_NEXT_CHAIN(vk->mVulkan.mKHRAccelerationStructureExtension, enabledAccelerationStructureFeatures);
#endif
#if VK_KHR_ray_query
	VkPhysicalDeviceRayQueryFeaturesKHR enabledRayQueryFeatures = {};
	enabledRayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
	enabledRayQueryFeatures.rayQuery = VK_TRUE;
	ADD_TO_NEXT_CHAIN(vk->mVulkan.mKHRRayQueryExtension, enabledRayQueryFeatures);
#endif
#endif
#ifdef NX64
	vkGetPhysicalDeviceFeatures2(vk->mVulkan.pVkActiveGPU, &gpuFeatures2);
#else
	vkGetPhysicalDeviceFeatures2KHR(vk->active_physical_device, &gpuFeatures2);
#endif

	// Get queue family properties
	uint32_t                 queueFamiliesCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vk->active_physical_device, &queueFamiliesCount, NULL);
	VkQueueFamilyProperties queueFamiliesProperties[queueFamiliesCount];
	vkGetPhysicalDeviceQueueFamilyProperties(vk->active_physical_device, &queueFamiliesCount, queueFamiliesProperties);

	// need a queue_priority for each queue in the queue family we create
	const uint32_t       kMaxQueueFamilies = 16;
	const uint32_t       kMaxQueueCount = 64;
	float                    queueFamilyPriorities[kMaxQueueFamilies][kMaxQueueCount] = {};
	uint32_t                 queue_create_infos_count = 0;
	VkDeviceQueueCreateInfo queue_create_infos[queueFamiliesCount];

	for (uint32_t i = 0; i < queueFamiliesCount; i++)
	{
		uint32_t queueCount = queueFamiliesProperties[i].queueCount;
		if (queueCount > 0)
		{
			queueCount = 1;
			queue_create_infos[queue_create_infos_count] = {};
			queue_create_infos[queue_create_infos_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_infos[queue_create_infos_count].pNext = NULL;
			queue_create_infos[queue_create_infos_count].flags = 0;
			queue_create_infos[queue_create_infos_count].queueFamilyIndex = i;
			queue_create_infos[queue_create_infos_count].queueCount = queueCount;
			queue_create_infos[queue_create_infos_count].pQueuePriorities = queueFamilyPriorities[i];
			queue_create_infos_count++;
		}
	}

	for(int i = 0; i < queueFamiliesCount; i++)
	{
		if(vk->queue_indices[QUEUE_GRAPHICS] == 0 && queueFamiliesProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			vk->queue_indices[QUEUE_GRAPHICS] = i;
			continue;
		}

		if(vk->queue_indices[QUEUE_TRANSFER] == 0 && (queueFamiliesProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0 && queueFamiliesProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			vk->queue_indices[QUEUE_TRANSFER] = i;
		}

		if(vk->queue_indices[QUEUE_COMPUTE] == 0 && queueFamiliesProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			vk->queue_indices[QUEUE_COMPUTE] = i;
		}

	}

#if defined(QUEST_VR)
	char oculusVRDeviceExtensionBuffer[4096];
	hook_add_vk_device_extensions(deviceExtensionCache, &extension_count, MAX_DEVICE_EXTENSIONS, oculusVRDeviceExtensionBuffer, sizeof(oculusVRDeviceExtensionBuffer));
#endif

	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pNext = &gpuFeatures2;
	create_info.flags = 0;
	create_info.queueCreateInfoCount = queue_create_infos_count;
	create_info.pQueueCreateInfos = queue_create_infos;
	create_info.enabledLayerCount = 0;
	create_info.ppEnabledLayerNames = NULL;
	create_info.enabledExtensionCount = extension_count;
	create_info.ppEnabledExtensionNames = device_extension_cache;
	create_info.pEnabledFeatures = NULL;

#if defined(ENABLE_NSIGHT_AFTERMATH)
	if (vk->mDiagnosticCheckPointsSupport && vk->mDiagnosticsConfigSupport)
	{
		vk->mAftermathSupport = true;
		LOGF(LogLevel::eINFO, "Successfully loaded Aftermath extensions");
	}

	if (vk->mAftermathSupport)
	{
		DECLARE_ZERO(VkDeviceDiagnosticsConfigCreateInfoNV, diagnostics_create_info);
		diagnostics_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV;
		diagnostics_create_info.flags = VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV |
			VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV |
			VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV;
		diagnostics_create_info.pNext = gpuFeatures2.pNext;
		gpuFeatures2.pNext = &diagnostics_create_info;
		// Enable Nsight Aftermath GPU crash dump creation.
		// This needs to be done before the Vulkan device is created.
		CreateAftermathTracker(vk->pName, &vk->mAftermathTracker);
	}
#endif

	/************************************************************************/
	// Add Device Group Extension if requested and available
	/************************************************************************/
#if VK_KHR_device_group_creation
	//ADD_TO_NEXT_CHAIN(vk->mGpuMode == GPU_MODE_LINKED, deviceGroupInfo);
#endif
	CHECK_VKRESULT(vkCreateDevice(vk->active_physical_device, &create_info, &vk_allocation_callbacks, &vk->device));

#if !defined(NX64)
	// Load Vulkan device functions to bypass loader
	//if (vk->mGpuMode != GPU_MODE_UNLINKED)
		volkLoadDevice(vk->device);
#endif
#endif
		vk->active_gpu_settings.dedicated_allocations =
			vk->active_gpu_settings.dedicated_allocations && vk->active_gpu_settings.memory_req_2_ext;

		if (vk->active_gpu_settings.dedicated_allocations)
		{
			SL_LOG_INFO("Successfully loaded Dedicated Allocation extension\n");
		}

#if VK_KHR_draw_indirect_count
		if (vk->active_gpu_settings.draw_indirect_count)
		{
			//PFN_VkCmdDrawIndirectCountKHR = vkCmdDrawIndirectCountKHR;
			//PFN_VkCmdDrawIndexedIndirectCountKHR = vkCmdDrawIndexedIndirectCountKHR;
			SL_LOG_INFO("Successfully loaded Draw Indirect extension\n");
		}
		else if (vk->active_gpu_settings.amd_draw_indirect_count)
#endif
		{
			//pfnVkCmdDrawIndirectCountKHR = vkCmdDrawIndirectCountAMD;
			//pfnVkCmdDrawIndexedIndirectCountKHR = vkCmdDrawIndexedIndirectCountAMD;
			SL_LOG_INFO("Successfully loaded AMD Draw Indirect extension\n");
		}

		if (vk->active_gpu_settings.amd_gcn_shader_extension)
		{
			SL_LOG_INFO("Successfully loaded AMD GCN Shader extension\n");
		}

		if (vk->active_gpu_settings.descriptor_indexing)
		{
			SL_LOG_INFO("Successfully loaded Descriptor Indexing extension\n");
		}

		if(vk->active_gpu_settings.buffer_device_address)
		{
			SL_LOG_INFO("Successfully loaded Buffer Device Address extension\n");
		}

		//TODO: RAYTRACING
		/*
		if (vk->mVulkan.mRaytracingSupported)
		{
			LOGF(LogLevel::eINFO, "Successfully loaded Khronos Ray Tracing extensions");
		}
		*/
#ifdef _ENABLE_DEBUG_UTILS_EXTENSION

		vk->mVulkan.mDebugMarkerSupport = (vkCmdBeginDebugUtilsLabelEXT) && (vkCmdEndDebugUtilsLabelEXT) && (vkCmdInsertDebugUtilsLabelEXT) &&
			(vkSetDebugUtilsObjectNameEXT);
#endif

		vkGetDeviceQueue(vk->device, vk->queue_indices[QUEUE_GRAPHICS], 0, &vk->graphics_queue);
		vkGetDeviceQueue(vk->device, vk->queue_indices[QUEUE_TRANSFER], 0, &vk->transfer_queue);
		vkGetDeviceQueue(vk->device, vk->queue_indices[QUEUE_COMPUTE], 0, &vk->compute_queue);

	return true;

}

bool vulkan_create_backend(sl_render_backend* backend, sl_allocator* allocator)
{
	vulkan_backend* vk = (vulkan_backend*)sl_alloc(allocator, sizeof(vulkan_backend));
	/*
	 * Initialization
	 */

	backend_allocator = allocator;
	vk->allocator = allocator;

	//TODO: Remove all alloca calls and use temp allocator instead.

	if(!vulkan_init_common(vk))
	{
		return false;
	}

	if(!vulkan_add_device(vk))
	{
		return false;
	}

	/************************************************************************/
	// Memory allocator
	/************************************************************************/
	VmaAllocatorCreateInfo createInfo = { 0 };
	createInfo.device = vk->device;
	createInfo.physicalDevice = vk->active_physical_device;
	createInfo.instance = vk->instance;

	if (vk->active_gpu_settings.dedicated_allocations)
	{
		createInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
	}

	if (vk->active_gpu_settings.buffer_device_address)
	{
		createInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	}

	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
	vulkanFunctions.vkAllocateMemory = vkAllocateMemory;
	vulkanFunctions.vkBindBufferMemory = vkBindBufferMemory;
	vulkanFunctions.vkBindImageMemory = vkBindImageMemory;
	vulkanFunctions.vkCreateBuffer = vkCreateBuffer;
	vulkanFunctions.vkCreateImage = vkCreateImage;
	vulkanFunctions.vkDestroyBuffer = vkDestroyBuffer;
	vulkanFunctions.vkDestroyImage = vkDestroyImage;
	vulkanFunctions.vkFreeMemory = vkFreeMemory;
	vulkanFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
	vulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
	vulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
	vulkanFunctions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR;
	vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
	vulkanFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
	vulkanFunctions.vkMapMemory = vkMapMemory;
	vulkanFunctions.vkUnmapMemory = vkUnmapMemory;
	vulkanFunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
	vulkanFunctions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
	vulkanFunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;
#if VMA_BIND_MEMORY2 || VMA_VULKAN_VERSION >= 1001000
	/// Fetch "vkBindBufferMemory2" on Vulkan >= 1.1, fetch "vkBindBufferMemory2KHR" when using VK_KHR_bind_memory2 extension.
	vulkanFunctions.vkBindBufferMemory2KHR = vkBindBufferMemory2KHR;
	/// Fetch "vkBindImageMemory2" on Vulkan >= 1.1, fetch "vkBindImageMemory2KHR" when using VK_KHR_bind_memory2 extension.
	vulkanFunctions.vkBindImageMemory2KHR = vkBindImageMemory2KHR;
#endif
#if VMA_MEMORY_BUDGET || VMA_VULKAN_VERSION >= 1001000
#ifdef NX64
	vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
#else
	vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR;
#endif
#endif
#if VMA_VULKAN_VERSION >= 1003000
	/// Fetch from "vkGetDeviceBufferMemoryRequirements" on Vulkan >= 1.3, but you can also fetch it from "vkGetDeviceBufferMemoryRequirementsKHR" if you enabled extension VK_KHR_maintenance4.
	vulkanFunctions.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
	/// Fetch from "vkGetDeviceImageMemoryRequirements" on Vulkan >= 1.3, but you can also fetch it from "vkGetDeviceImageMemoryRequirementsKHR" if you enabled extension VK_KHR_maintenance4.
	vulkanFunctions.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;
#endif

	createInfo.pVulkanFunctions = &vulkanFunctions;
	createInfo.pAllocationCallbacks = &vk_allocation_callbacks;
	vmaCreateAllocator(&createInfo, &vk->vma_allocator);
		/*
		 * Set the function pointers
		 */
	backend->create_swapchain = vulkan_create_swapchain;
	backend->destroy_swapchain = vulkan_destroy_swapchain;
	backend->present_swapchain = vulkan_present_swapchain;



	backend->inst = vk;
	return true;
}

void vulkan_destroy_backend(sl_render_backend* backend)
{
	vulkan_backend* vk = (vulkan_backend*)backend->inst;

	vmaDestroyAllocator(vk->vma_allocator);
//
	vkDestroyDevice(vk->device, &vk_allocation_callbacks);
//
#ifdef ENABLE_DEBUG_UTILS_EXTENSION
	if (vk->debug_messenger)
	{
		vkDestroyDebugUtilsMessengerEXT(vk->instance, vk->debug_messenger, &vk_allocation_callbacks);
		vk->debug_messenger = NULL;
	}
#else
	if (vk->debug_report)
	{
		vkDestroyDebugReportCallbackEXT(vk->instance, vk->debug_report, &vk_allocation_callbacks);
		vk->debug_report = NULL;
	}
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