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
//
// Created by Jonah Goldsmith on 1/3/23.
//
#include "os_window.h"

#if SL_PLATFORM_OSL

#include "base/logging/logger.h"
#include "registry/plugin_util.inl"
#include "base/logging/logger.h"

#include "base/memory/allocator.h"

#include "GLFW/glfw3.h"

static struct sl_logger_api* sl_logger_api;

static sl_allocator* window_allocator;

static os_window* macos_create_window(os_window_desc* p_desc)
{
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Starlight", 0, 0);
	return (os_window*)window;
}

void* window_alloc(size_t size, void* user_data)
{
	void* ptr = sl_alloc(window_allocator, size);
	return ptr;
}

void* window_realloc(void* ptr, size_t size, void* user_data)
{
	void* out = sl_realloc(window_allocator, ptr, size);
	return out;
}

void window_free(void* ptr, void* user_data)
{
	sl_free(window_allocator, ptr);
}

static void macos_init_window_system(sl_allocator* allocator)
{
	window_allocator = allocator;

	GLFWallocator glfw_alloc = {
		.allocate = window_alloc,
		.deallocate = window_free,
		.reallocate = window_realloc
	};
	glfwInitAllocator(&glfw_alloc);
	glfwInit();
}

static void macos_poll_events(void)
{
	glfwPollEvents();
}

static bool macos_should_window_close(os_window* win)
{
	GLFWwindow* glfw = (GLFWwindow *)win;

	if(glfwWindowShouldClose(glfw))
	{
		return true;
	}
	return false;
}

void macos_destroy_window(os_window* win)
{
	GLFWwindow* glfw = (GLFWwindow *)win;

	glfwDestroyWindow(glfw);

}

void macos_shutdown_window_system(void)
{
	glfwTerminate();
}

static struct os_window_api macos_api = {
	.create_window = macos_create_window,
	.init_window_system = macos_init_window_system,
	.poll_events = macos_poll_events,
	.should_window_close = macos_should_window_close,
	.destroy_window = macos_destroy_window,
	.shutdown_window_system = macos_shutdown_window_system,
};

PLUGIN_EXPORT int sl_load_plugin(struct sl_api_registry* reg, struct cr_plugin *ctx, enum cr_op operation)
{

	switch(operation) {
	case CR_LOAD:
		reg->set(OS_WINDOW_API, &macos_api, sizeof(struct os_window_api));
		sl_logger_api = reg->get(SL_LOGGER_API);
		SL_LOG_INFO("Testing Hot Reloading\n");
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

#endif