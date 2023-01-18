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

#include "mem_tracker.h"
#include "allocator.h"
#include "data_structures/array.inl"
#include "data_structures/hash.inl"
#include "thread/atomics.inl"
#include "thread/mutex.inl"
#include "util/assertions.inl"
#include "util/sprintf.h"
#include "logging/logger.h"

/*DISCLAIMER: MEMORY TRACKER BASED OFF OF
https://github.com/nothings/stb/blob/master/stb_leakcheck.h
https://github.com/niklasfrykholm/stingray-engine-code-walkthrough - MEMORY SYSTEM
https://github.com/septag/rizz MEMORY CONTEXT SYSTEM
https://github.com/rxi/dmt
http://bitsquid.blogspot.com/2010/09/custom-memory-allocation-in-c.html ProxyAllocator
https://github.com/memtt/malt
*/

#define MAX_CONTEXTS 512

typedef struct trace_map
{
    uint64_t key;
    uint32_t value;
}trace_map;

typedef struct ptr_map
{
    void* key;
    uint32_t value;
}ptr_map;

typedef struct internal_memory_tracker
{
    sl_allocator allocator;
    uint32_t num_contexts;

    sl_os_mutex tracker_mutex;

    //TODO: should this be bigger?
    sl_memory_tracker_context contexts[MAX_CONTEXTS];

    SL_ARRAY(uint32_t*, context_list);
    SL_ARRAY(struct sl_memory_tracker_trace*, traces);

    ptr_map* ptr_map;
    trace_map* trace_map;

}internal_memory_tracker;

static internal_memory_tracker* internal_tracker;

extern struct sl_allocator_api* sl_allocator_api; //allocator.c
extern struct sl_sprintf_api* sl_sprintf_api; //sprintf.c
extern struct sl_logger_api* sl_logger_api; //logger.c



static void print_traces(uint32_t context)
{
	SL_MUTEX_LOCK(internal_tracker->tracker_mutex) {
		for (uint32_t i = 1; i < (uint32_t)sl_array_size(internal_tracker->traces); i++) {
			const struct sl_memory_tracker_trace *t = internal_tracker->traces + i;
			if (t->context == context && t->amount_allocated) {
				const struct sl_memory_tracker_trace cur_trace = internal_tracker->traces[i];
				sl_unlock_mutex(&internal_tracker->tracker_mutex);
				SL_LOG_INFO("Leaked %llu bytes. File %s:%i in function |%s|\n", cur_trace.amount_allocated,  cur_trace.file,  cur_trace.line, cur_trace.func);
				sl_lock_mutex(&internal_tracker->tracker_mutex);
			}
		}
	}
}

static uint32_t create_context(const char *name, uint32_t parent);
static void record(void *old_ptr, size_t old_size, void *new_ptr, size_t new_size,
				   const char* func, const char *file, uint32_t line, uint32_t context);
static void set_context_tracking(uint32_t context, bool enabled);

void sl_init_memory_tracker(void)
{
    sl_allocator allocator = *sl_allocator_api->system;
    allocator.context = SL_MEMORY_CONTEXT_NONE;
    internal_tracker = sl_alloc(&allocator, sizeof(internal_memory_tracker));

    *internal_tracker = (internal_memory_tracker){
            .allocator = allocator
    };
    sl_create_mutex(&internal_tracker->tracker_mutex);

    create_context("root", 0);
    const uint32_t mem_tracker_context = create_context("memory_tracker", SL_MEMORY_CONTEXT_NONE);


    set_context_tracking(mem_tracker_context, false);
    internal_tracker->allocator.context = mem_tracker_context;

    sl_array_push(&internal_tracker->allocator, internal_tracker->traces, (struct sl_memory_tracker_trace){ 0 });

    record(0, 0, internal_tracker, sizeof(internal_memory_tracker), SL_FUNCTION, __FILE__, __LINE__, mem_tracker_context);
}

static uint32_t create_context(const char *name, uint32_t parent)
{
    struct sl_memory_tracker_context *context;
    SL_MUTEX_LOCK(internal_tracker->tracker_mutex) {
        context = sl_array_size(internal_tracker->context_list) ? internal_tracker->contexts + sl_array_pop(internal_tracker->context_list) :
                              internal_tracker->num_contexts < MAX_CONTEXTS ? internal_tracker->contexts + internal_tracker->num_contexts++ : 0;
        if (context > internal_tracker->contexts && parent != SL_MEMORY_CONTEXT_NONE)
            internal_tracker->contexts[parent].num_children += 1;
    }

	SL_ASSERT(context, "Too many contexts");

    *context = (sl_memory_tracker_context){
            .name = name,
            .parent_context = parent,
            .tracking_enabled = true,
    };
    return (uint32_t)(context - internal_tracker->contexts);
}

static void destroy_context(uint32_t context)
{

    uint32_t parent;
    {
        const sl_memory_tracker_context *c = internal_tracker->contexts + context;
        if (c->amount_allocated && c->tracking_enabled) {
            print_traces(context);
        }

        if (c->amount_allocated) {
            //TODO: ASSERT AND CALL AN ERROR FUNC
        }
        if (c->num_children) {
            //TODO: ASSERT AND CALL ERROR THAT THE SCOPE HAS UNCLOSED SUB SCOPES
            for (const sl_memory_tracker_context *p_context = internal_tracker->contexts; p_context != sl_array_end(internal_tracker->contexts); ++p_context) {
                if (p_context && p_context->parent_context == context) {
                    //TODO: ERROR FOUND UNCLOSED SUBSCOPE!
                }
            }
        }

        parent = c->parent_context;
        internal_tracker->contexts[context] = (sl_memory_tracker_context){ 0 };
    }

    SL_MUTEX_LOCK(internal_tracker->tracker_mutex) {
        sl_array_push(&internal_tracker->allocator, internal_tracker->context_list, context);
        if (context && parent != SL_MEMORY_CONTEXT_NONE)
            internal_tracker->contexts[parent].num_children -= 1;
    }

}

static void mem_trace(void *ptr, size_t size, const char* func, const char *file, uint32_t line, uint32_t context)
{
    const char* before = file + ((uint64_t)line) + context;
    const uint64_t key = sl_hash_bytes(&before, strlen(before), 0);

    SL_MUTEX_LOCK(internal_tracker->tracker_mutex) {
        uint32_t cur_trace = sl_hashmap_get(&internal_tracker->allocator, internal_tracker->trace_map, key);
        if (!cur_trace) {
            cur_trace = (uint32_t) sl_array_size(internal_tracker->traces);
            struct sl_memory_tracker_trace trace = {
                    .func = func,
                    .file = file,
                    .line = line,
                    .context = context,
					.ptr = ptr,
            };
            sl_array_push(&internal_tracker->allocator, internal_tracker->traces, trace);
            sl_hashmap_push(&internal_tracker->allocator, internal_tracker->trace_map, key, cur_trace);
        }
        struct sl_memory_tracker_trace *trace = internal_tracker->traces + cur_trace;
        trace->amount_allocated += size;
        sl_hashmap_push(&internal_tracker->allocator, internal_tracker->ptr_map, ptr, cur_trace);
        internal_tracker->contexts[context].num_traces += 1;
    }

}

static void mem_untrace(void *ptr, size_t size, uint32_t context)
{
    SL_MUTEX_LOCK(internal_tracker->tracker_mutex) {
        const uint32_t cur_trace = sl_hashmap_get(&internal_tracker->allocator, internal_tracker->ptr_map, ptr);
        if (cur_trace) {
            struct sl_memory_tracker_trace *trace = internal_tracker->traces + cur_trace;
            trace->amount_allocated -= size;
            sl_hashmap_del(&internal_tracker->allocator, internal_tracker->ptr_map, ptr);
            internal_tracker->contexts[context].num_traces -= 1;
        }
    }
}

static void record(void *old_ptr, size_t old_size, void *new_ptr, size_t new_size,
                           const char* func, const char *file, uint32_t line, uint32_t context)
{
    if (context == SL_MEMORY_CONTEXT_NONE)
        return;

    sl_memory_tracker_context *c = internal_tracker->contexts + context;
    atomic_fetch_add(&c->amount_allocated, (new_size - old_size));

    const size_t old_count = (old_size > 0) ? 1 : 0;
    const size_t new_count = (new_size > 0) ? 1 : 0;
    atomic_fetch_add(&c->allocation_count, (new_count - old_count));

    SL_ASSERT(c->amount_allocated < 0xf000000000000000ULL, "Negative Byte Count!");

    if (old_size > 0 && (c->tracking_enabled || c->num_traces)) {
		mem_untrace(old_ptr, old_size, context);
	}

    if (new_size > 0 && c->tracking_enabled) {
		mem_trace(new_ptr, new_size, func, file, line, context);
	}
}


static uint64_t get_total_bytes(uint32_t context)
{
    return internal_tracker->contexts[context].amount_allocated;
}

static uint64_t get_total_count(uint32_t context)
{
    return internal_tracker->contexts[context].allocation_count;
}

static void set_context_tracking(uint32_t context, bool enabled)
{
    internal_tracker->contexts[context].tracking_enabled = enabled;
}

static struct sl_memory_tracker_trace *trace_data()
{
	// We need to allocate the array before we take the critical section, because the allocation
	// could potentially call back into the memory tracker.

	struct sl_memory_tracker_trace *res = 0;

	sl_array_resize(&internal_tracker->allocator, res, sl_array_size(internal_tracker->traces));

	SL_MUTEX_LOCK(internal_tracker->tracker_mutex)
	{
		sl_memcpy(res, internal_tracker->traces, sl_array_bytes(res));
	}

	return res;
}

static struct sl_memory_tracker_context *scope_data()
{
	// We need to allocate the array before we take the critical section, because the allocation
	// could potentially call back into the memory tracker.

	struct sl_memory_tracker_context *res = 0;

	sl_array_resize(&internal_tracker->allocator, res, internal_tracker->num_contexts);

	SL_MUTEX_LOCK(internal_tracker->tracker_mutex)
	{
		sl_memcpy(res, internal_tracker->contexts, sl_array_bytes(res));
	}

	return res;
}

const char* context_name(uint32_t context)
{
	return internal_tracker->contexts[context].name;
}


static void check_for_leaks(void)
{
    if (internal_tracker->num_contexts != (uint32_t)sl_array_size(internal_tracker->context_list) + 1) {
        for (uint32_t i = 1; i < internal_tracker->num_contexts; ++i) {
            sl_memory_tracker_context *context = internal_tracker->contexts + i;

            if (context->parent_context != SL_MEMORY_CONTEXT_NONE) {
				char buffer[1024];
				sl_sprintf(buffer, "Memory Context %s Still Open", context->name);
				SL_ASSERT(context->name == NULL, buffer);
            }
        }
    }
}

static struct sl_memory_tracker_api mem_api = {
	.create_context = create_context,
	.destroy_context = destroy_context,
	.record = record,
	.check_for_leaks = check_for_leaks,
	.print_traces = print_traces,
	.toggle_tracking = set_context_tracking,
	.trace_data = trace_data,
	.context_name = context_name,
	.scope_data = scope_data,
};

struct sl_memory_tracker_api* sl_memory_tracker_api = &mem_api;
