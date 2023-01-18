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
#include "base/data_structures/array.inl"
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

struct sl_job_system_api* job_api;
struct os_window_api* window_api;
static sl_allocator job_alloc;
static sl_allocator window_alloc;

typedef struct sl_run_state
{
	os_window* main_window;
}sl_run_state;

bool tick(void* data)
{
	sl_run_state* state = (sl_run_state*)data;

	while(!window_api->should_window_close(state->main_window))
	{
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

	os_window* win = window_api->create_window(0);

	out->main_window = win;

	return out;

}

int main(int argc, char** argv)
{
	sl_run_state* app = application_init(argc, argv);

	while(tick(app))
	{
		sl_plugin_system_api->check_hot_reload();
	}

	sl_allocator_api->destroy_child(&window_alloc);

	sl_shutdown_plugin_system();

	sl_destroy_job_system();

	sl_allocator_api->destroy_child(&job_alloc);

	sl_memory_tracker_api->check_for_leaks();

	return 0;
}