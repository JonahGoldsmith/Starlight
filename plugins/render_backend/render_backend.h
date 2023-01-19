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

#ifndef RENDER_BACKEND_H
#define RENDER_BACKEND_H

typedef struct sl_swapchain sl_swapchain;

typedef struct sl_window_handle
{
	void* handle;
}sl_window_handle;

typedef struct sl_swapchain_desc
{
	sl_window_handle handle;
	uint32_t width, height;
}sl_swapchain_desc;

typedef struct sl_render_backend
{
	void* inst;

	sl_swapchain* (*create_swapchain)(struct sl_render_backend* backend, sl_swapchain_desc* desc);

	void (*destroy_swapchain)(struct sl_render_backend* backend, sl_swapchain* swapchain);

	void (*present_swapchain)(struct sl_render_backend* backend, sl_swapchain* swapchain);

	void (*resize_swapchain)(struct sl_render_backend* backend, sl_swapchain* swapchain, int width, int height);

}sl_render_backend;

#endif//RENDER_BACKEND_H
