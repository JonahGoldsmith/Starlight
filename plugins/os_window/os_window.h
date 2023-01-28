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

#ifndef OS_WINDOW_H
#define OS_WINDOW_H

#include "base/defines.h"

struct sl_allocator;

typedef struct os_window os_window;

typedef struct os_window_handle
{
	void* handle;
	void* layer;
}os_window_handle;

typedef struct os_window_desc
{
	const char* name;
	uint32_t x, y, width, height;
}os_window_desc;

typedef void (*window_resize_cb_fn)(os_window* window, int width, int height);

struct os_window_api
{
	void (*init_window_system)(struct sl_allocator* allocator);

	os_window* (*create_window)(os_window_desc* p_desc);

	void (*poll_events)(void);

	bool (*should_window_close)(os_window* p_window);

	void (*destroy_window)(os_window* p_window);

	void (*shutdown_window_system)(void);

	os_window_handle (*get_native_handle)(os_window* p_window);

	void (*set_window_resize_callback)(os_window* p_window, window_resize_cb_fn cb);

};

#define OS_WINDOW_API "os_window_api"

#endif//OS_WINDOW_H
