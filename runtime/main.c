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
