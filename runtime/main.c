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

#include <printf.h>
#include "base/defines.h"
#include "base/memory/allocator.h"
#include "base/data_structures/array.inl"
#include "base/data_structures/hash.inl"

/*
uint32_t* numbers = NULL;

struct linear_allocator
{
    size_t total_size;
    void* start_ptr;
    size_t offset;
};

void init_linear_allocator(struct linear_allocator* allocator, sl_allocator* a, size_t total_size)
{
    allocator->total_size = total_size;
    allocator->offset = 0;
    allocator->start_ptr = sl_alloc(a, total_size);
}

void* linear_alloc(struct linear_allocator* a, size_t size)
{
    size_t cur_address = (size_t)a->start_ptr + a->offset;
    size_t next_address = cur_address;
    a->offset += size;
    return (void*)next_address;
}

void linear_reset(struct linear_allocator* a)
{
    a->offset = 0;
}

SL_THREAD_LOCAL struct linear_allocator alloc;

typedef struct temp_allocator_1024
{
    char buffer[1024];
}temp_allocator_1024;



struct temp_allocator_item
{
    size_t size;
    size_t last_used;
    size_t used;
    struct temp_allocator_item* next;
};

typedef struct temp_allocator_o
{
    char* buffer;
    struct linear_allocator* backing;
    struct temp_allocator_item* first;
}temp_allocator_o;

void* temp_realloc(struct temp_allocator_o* data, void* ptr, size_t old_size, size_t new_size)
{
    struct temp_allocator_item* item = (struct temp_allocator_item *) data->first;
    // Is this free
    if (new_size == 0)
        return NULL;
    // Does it fit in the block
    if (item && item->size - item->used >= new_size) {
        void *res = (char *)item + item->used;
        item->last_used = item->used;
        item->used += new_size;
        if(!ptr)
        {
            return res;
        }
        if (res != ptr && old_size && new_size)
            memcpy(res, ptr, old_size < new_size ? old_size : new_size);
        return res;
    }else if(ptr && new_size) {
        void *res = linear_alloc(data->backing, new_size);
        memcpy(res, ptr, old_size);
        return res;
    }else
        return linear_alloc(data->backing, new_size);

}

typedef struct temp_allocator
{
    struct temp_allocator_o* inst;
    void* (*realloc)(struct temp_allocator_o* data, void* ptr,         size_t old_size, size_t new_size);
}temp_allocator;

temp_allocator* create_temp_allocator_with_buffer(void* buffer, size_t size)
{
    temp_allocator *ta = (temp_allocator *)buffer;
    *ta = (temp_allocator){
            .inst = (struct temp_allocator_o *)buffer+ sizeof(temp_allocator),
            .realloc = temp_realloc,
    };
    ta->inst->buffer = buffer + sizeof(temp_allocator)+sizeof(temp_allocator_o);
    ta->inst->backing = &alloc;
    ta->inst->first = (struct temp_allocator_item *)(ta->inst->buffer);
    ta->inst->first->size = size;
    ta->inst->first->last_used = sizeof(struct temp_allocator_item);
    ta->inst->first->used = sizeof(struct temp_allocator_item) + sizeof(temp_allocator) + sizeof(temp_allocator_o);
    return ta;

}
*/

#include "base/thread/job_system.h"
#include "memory/mem_tracker.h"
#include "os/os.h"

void tick(void* data)
{

	char* b = sl_alloc(sl_allocator_api->system, 100);
	char* v = sl_alloc(sl_allocator_api->system, 100);
	char* x = sl_alloc(sl_allocator_api->system, 100);
	char* z = sl_alloc(sl_allocator_api->system, 100);

	char* and = sl_realloc(sl_allocator_api->system, b, 200);


	struct sl_memory_tracker_context* contexts = sl_memory_tracker_api->scope_data();
	for(int i = 0; i < sl_array_size(contexts); i++)
	{
		printf("Context %s: Has %llu allocated, with %u total allocations and %u child contexts\n", contexts[i].desc, contexts[i].amount_allocated, contexts[i].allocation_count, contexts[i].num_children);
	}
}

int main()
{
	sl_init_memory_tracker();

	sl_allocator alloc = *sl_allocator_api->system;
	alloc.context = sl_memory_tracker_api->create_context("host", 0);
	sl_memory_tracker_api->toggle_tracking(alloc.context, true);

	sl_job_system_desc desc = {
		.p_allocator = &alloc,
		.num_fibers = 128,
		.num_threads = sl_os_api->info->num_logical_cores()-1,
		.extended_stack_size = SL_KILOBYTES(512),
		.normal_stack_size = SL_KILOBYTES(64),
	};

	sl_os_api->thread->set_thread_name("Main Thread");

	struct sl_job_system_api* job_api = sl_create_job_system(&desc);

	sl_job_decl j = {
		.task = tick,
		.data = 0,
		.pinned_index = job_api->get_pin_index(0),
		.priority = sl_normal_priority };
	sl_job_counter *completed = job_api->run_jobs(&j, 1, sl_ss_normal);
	job_api->wait_for_counter_os(completed, 0.01);

	sl_destroy_job_system();

	sl_memory_tracker_api->destroy_context(alloc.context);
	sl_memory_tracker_api->check_for_leaks();


	return 0;
}