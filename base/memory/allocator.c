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

#include "allocator.h"
#include "base/thread/atomics.inl"
#include "mem_tracker.h"

extern struct sl_memory_tracker_api* sl_memory_tracker_api; //mem_tracker.c

#include <stdlib.h>

#if SL_PLATFORM_OSL
#define MIN_ALLOC_ALIGNMENT 16
#elif defined(_WIN64)
#endif

typedef struct mem_block {
    size_t size;
} mem_block;

static sl_allocator_statistics stats;

void* system_malloc(struct sl_allocator *a, size_t new_size, uint32_t align, const char* func,
                    const char *file, uint32_t line)
{
#pragma message(sl_reminder "Come Back For Windows !")
    mem_block* p = NULL;
    int res = posix_memalign((void **) &p, align, new_size+sizeof(mem_block));
    if(p == NULL)
        return p;
    atomic_fetch_add(&stats.total_allocation_count, 1);
    atomic_fetch_add(&stats.total_amount_allocated, new_size);
	sl_memory_tracker_api->record(0, 0, p, new_size, func, file, line, a->context);
    p->size = (int)new_size;
    return p + 1;
}

void system_free(struct sl_allocator* a, void* ptr, uint32_t align, const char* func, const char* file, uint32_t line)
{
    if (ptr != NULL) {
        mem_block * mi = (mem_block *)ptr - 1;
        atomic_fetch_add(&stats.total_allocation_count, -1);
        atomic_fetch_add(&stats.total_amount_allocated, -mi->size);
		sl_memory_tracker_api->record(mi, mi->size, 0, 0, func, file, line, a->context);
        free(mi);
    }

}

void* system_realloc(struct sl_allocator *a, void *ptr, size_t new_size, uint32_t align, const char* func,
                     const char *file, uint32_t line)
{
    if(align <= MIN_ALLOC_ALIGNMENT)
        align = MIN_ALLOC_ALIGNMENT;

    if (ptr == NULL) {
        return system_malloc(a, new_size, align, func, file, line);
    } else if (new_size == 0) {
        system_free(a, ptr, align, func, file, line);
        return NULL;
    } else {
        mem_block * mi = (mem_block *)ptr - 1;
        if (new_size <= mi->size) {
            return ptr;
        } else {
            void* q = system_malloc(a, new_size, align, func, file, line);
            if (q) {
                sl_memcpy(q, ptr, mi->size);
                system_free(a, ptr, align, func, file, line);
            }
            return q;
        }
    }

}

static sl_allocator create_child(const sl_allocator *parent, const char *name)
{
	sl_allocator a = *parent;
	a.context = sl_memory_tracker_api->create_context(name, parent->context);
	return a;
}

static void destroy_child(const sl_allocator *child)
{
	sl_memory_tracker_api->destroy_context(child->context);
}

struct sl_allocator system_allocator = {
        .realloc = system_realloc
};

struct sl_allocator_api alloc_api = {
    .system = &system_allocator,
    .stats = &stats,
	.create_child = create_child,
	.destroy_child = destroy_child,
};

struct sl_allocator_api* sl_allocator_api = &alloc_api;