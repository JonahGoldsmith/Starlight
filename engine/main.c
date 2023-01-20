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

#include "base/defines.h"
#include "base/memory/allocator.h"
#include "base/thread/job_system.h"
#include "memory/mem_tracker.h"
#include "os/os.h"
#include "logging/logger.h"
#include "registry/plugin_system.h"
#include "util/path_util.inl"
#include "register_engine_apis.h"
#include "registry/api_registry.h"
#include "plugins/os_window/os_window.h"

#include "plugins/render_backend/render_backend.h"
#include "plugins/render_backend_metal/render_backend_metal.h"
#include "plugins/render_backend_vulkan/render_backend_vulkan.h"
#include "util/assertions.inl"

struct sl_job_system_api* job_api;
struct os_window_api* window_api;

//#define METAL_API
#ifdef METAL_API
struct sl_render_backend_metal_api* render_api;
#endif

#define VULKAN_API
#ifdef VULKAN_API
struct sl_render_backend_vulkan_api* render_api;
#endif

static sl_allocator job_alloc;
static sl_allocator render_backend_alloc;
static sl_allocator window_alloc;
static sl_render_backend backend;
static sl_swapchain* swapchain;
static os_window* main_window;

typedef struct sl_run_state
{
	os_window* main_window;
}sl_run_state;

void resize_callback(os_window* window, int width, int height)
{
	os_window_handle handle = window_api->get_native_handle(main_window);

	sl_swapchain_desc swap_desc = {
		.handle = handle.handle
	};

	backend.destroy_swapchain(&backend, swapchain);
	swapchain = backend.create_swapchain(&backend, &swap_desc);
}

bool tick(void* data)
{
	sl_run_state* state = (sl_run_state*)data;

	while(!window_api->should_window_close(state->main_window))
	{
		backend.present_swapchain(&backend, swapchain);
		window_api->poll_events();
		return true;
	}

	window_api->shutdown_window_system();

	return false;
}

sl_run_state* application_init(int argc, char** argv)
{
	sl_init_memory_tracker();

	init_logger_system();

	sl_run_state* out = sl_alloc(sl_allocator_api->system, sizeof(sl_run_state));

	job_alloc = sl_allocator_api->create_child(sl_allocator_api->system, "job_system");

	sl_job_system_desc desc = {
		.p_allocator = &job_alloc,
		.num_fibers = 128,
		.num_threads = sl_os_api->info->num_logical_cores()-1,
		.extended_stack_size = SL_KILOBYTES(512),
		.normal_stack_size = SL_KILOBYTES(64),
	};

	sl_os_api->thread->set_thread_name("Main Thread");

	job_api = sl_create_job_system(&desc);

	register_engine_apis();

	sl_init_plugin_system();

	//TODO Put this somewhere else
	char temp_path[1024];
	char back[1024];
	char plugin_dir[1024];
	sl_get_executable_path(temp_path, 1024);
	sl_get_one_dir_back(temp_path, back);
	memset(temp_path, 0, 1024);
	sl_concat_dir("plugins", back, plugin_dir);
	sl_concat_dir_end_slash("temp", back, temp_path);

	sl_plugin_system_api->load_all_plugins(plugin_dir, temp_path);

	sl_plugin_system_api->check_hot_reload();

	window_api = sl_global_api_registry->get(OS_WINDOW_API);

	window_alloc = sl_allocator_api->create_child(sl_allocator_api->system, "window_system");

	window_api->init_window_system(&window_alloc);

	main_window = window_api->create_window(0);

	out->main_window = main_window;

	//Rendering!
#ifdef METAL_API
		render_api = sl_global_api_registry->get(RENDER_BACKEND_METAL_API);
#elif defined(VULKAN_API)
	render_api = sl_global_api_registry->get(RENDER_BACKEND_VULKAN_API);
#endif

	render_backend_alloc = sl_allocator_api->create_child(sl_allocator_api->system, "render_backend");

	render_api->create_backend(&backend, &render_backend_alloc);

	os_window_handle handle = window_api->get_native_handle(main_window);

	sl_swapchain_desc swap_desc = {
		.handle = handle.handle
	};

	swapchain = backend.create_swapchain(&backend, &swap_desc);
	//END RENDERING

	window_api->set_window_resize_callback(main_window, resize_callback);

	return out;

}

int main(int argc, char** argv)
{
	sl_run_state* app = application_init(argc, argv);

	while(tick(app))
	{
		sl_plugin_system_api->check_hot_reload();
	}

	backend.destroy_swapchain(&backend, swapchain);

	render_api->destroy_backend(&backend);

	sl_allocator_api->destroy_child(&render_backend_alloc);

	sl_allocator_api->destroy_child(&window_alloc);

	sl_shutdown_plugin_system();

	sl_destroy_job_system();

	sl_allocator_api->destroy_child(&job_alloc);

	sl_memory_tracker_api->check_for_leaks();

	return 0;
}
