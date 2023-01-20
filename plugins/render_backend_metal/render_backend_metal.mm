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

#include "render_backend_metal.h"
#include "base/memory/allocator.h"
#include "registry/plugin_util.inl"
#include "render_backend/render_backend.h"
#import "thread/mutex.inl"
#import "util/assertions.inl"

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include "metal_availability_macros.h"
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#define NOREFS __unsafe_unretained

struct metal_backend
{
	id<MTLDevice> gpu;
	id<MTLCommandQueue> graphics_queue;
	sl_allocator* allocator;
	struct VmaAllocator_T* vma_allocator;
	NOREFS id<MTLHeap>* heaps API_AVAILABLE(macos(10.13), ios(10.0));
	uint32_t                   heap_count;
	uint32_t                   heap_capacity;
	// To synchronize resource allocation done through automatic heaps
	sl_os_mutex*   heap_mutex;
};

struct metal_swapchain
{
	CAMetalLayer* layer;
	sl_swapchain_desc desc;
};

#pragma region Metal_Backend_Interface

sl_swapchain* metal_create_swapchain(struct sl_render_backend* backend, sl_swapchain_desc* desc)
{
	metal_backend* mtl = static_cast<metal_backend *>(backend->inst);
	sl_swapchain* out;
	metal_swapchain* mtl_swapchain = static_cast<metal_swapchain *>(sl_alloc(mtl->allocator, sizeof(metal_swapchain)));
	NSWindow* window = ((__bridge NSWindow*)(desc->handle.handle));
	mtl_swapchain->layer = (CAMetalLayer*)window.contentView.layer;
	mtl_swapchain->layer.device = mtl->gpu;
	window.contentView.autoresizesSubviews = YES;
	mtl_swapchain->desc = *desc;
	out = reinterpret_cast<sl_swapchain *>(mtl_swapchain);
	return out;
}

void metal_destroy_swapchain(struct sl_render_backend* backend, sl_swapchain* swapchain)
{
	metal_backend* mtl = static_cast<metal_backend *>(backend->inst);
	metal_swapchain* mtl_swapchain = reinterpret_cast<metal_swapchain *>(swapchain);

	sl_free(mtl->allocator, mtl_swapchain);

}

void metal_present_swapchain(struct sl_render_backend* backend, sl_swapchain* swapchain)
{
	metal_backend* mtl = static_cast<metal_backend *>(backend->inst);
	metal_swapchain* mtl_swapchain = reinterpret_cast<metal_swapchain *>(swapchain);

	@autoreleasepool {

		id<CAMetalDrawable> surface = [mtl_swapchain->layer nextDrawable];

		MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
		pass.colorAttachments[0].clearColor = {1, 0, 0, 1};
		pass.colorAttachments[0].loadAction  = MTLLoadActionClear;
		pass.colorAttachments[0].storeAction = MTLStoreActionStore;
		pass.colorAttachments[0].texture = surface.texture;

		id<MTLCommandBuffer> buffer = [mtl->graphics_queue commandBuffer];
		id<MTLRenderCommandEncoder> encoder = [buffer renderCommandEncoderWithDescriptor:pass];
		[encoder endEncoding];
		[buffer presentDrawable:surface];
		[buffer commit];
	}

}

#pragma endregion

#pragma region Metal_Backend_Initialization
bool metal_create_backend(sl_render_backend* backend, sl_allocator* allocator)
{
	metal_backend* out = static_cast<metal_backend *>(sl_alloc(allocator, sizeof(metal_backend)));
	out->allocator = allocator;
	out->gpu = MTLCreateSystemDefaultDevice();
	out->graphics_queue = [out->gpu newCommandQueueWithMaxCommandBufferCount:512];
	SL_ASSERT(out->gpu, "Failed to create default Metal device!");
	if(!out->gpu)
		return false;

	backend->inst = out;

	backend->create_swapchain = metal_create_swapchain;
	backend->destroy_swapchain = metal_destroy_swapchain;
	backend->present_swapchain = metal_present_swapchain;

	return true;
}


void metal_destroy_backend(sl_render_backend* backend)
{
	metal_backend* mtl = static_cast<metal_backend *>(backend->inst);
	sl_free(mtl->allocator, mtl);
}



#pragma endregion

static sl_render_backend_metal_api metal_backend_api = {
	.create_backend = metal_create_backend,
	.destroy_backend = metal_destroy_backend,
};

PLUGIN_EXPORT int sl_load_plugin(struct sl_api_registry* reg, struct cr_plugin *ctx, enum cr_op operation)
{

	switch(operation) {
	case CR_LOAD:
		reg->set(RENDER_BACKEND_METAL_API, &metal_backend_api, sizeof(struct sl_render_backend_metal_api));
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