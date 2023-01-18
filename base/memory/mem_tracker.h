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


#ifndef STARLIGHT_MEM_TRACKER_H
#define STARLIGHT_MEM_TRACKER_H

#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SL_MEMORY_CONTEXT_NONE 0xffffffffu

typedef struct sl_memory_tracker_context {
	const char *name;

	SL_ATOMIC uint64_t amount_allocated;

	SL_ATOMIC uint32_t allocation_count;

	uint32_t parent_context;

	uint32_t num_children;

	bool tracking_enabled;

	uint32_t num_traces;

} sl_memory_tracker_context;

struct sl_memory_tracker_trace {
	const char *func;

	const char *file;

	uint32_t line;

	uint32_t context;

	uint64_t amount_allocated;

	void *ptr;
};

struct sl_memory_tracker_api {
	void (*check_for_leaks)(void);

	uint32_t (*create_context)(const char *name, uint32_t parent);

	void (*destroy_context)(uint32_t context);

	void (*record)(void *old_ptr, size_t old_size, void *new_ptr, size_t new_size, const char *func, const char *file, uint32_t line, uint32_t context);

	void (*print_traces)(uint32_t context);

	void (*toggle_tracking)(uint32_t context, bool enabled);

	struct sl_memory_tracker_trace *(*trace_data)(void);

	struct sl_memory_tracker_context *(*scope_data)(void);

	const char *(*context_name)(uint32_t context);
};

#define SL_MEM_TRACKER_API "sl_memory_tracker_api"

#ifdef LINKS_SL_BASE

extern struct sl_memory_tracker_api *sl_memory_tracker_api;

extern void sl_init_memory_tracker(void);

#endif

#ifdef __cplusplus
}
#endif

#endif //STARLIGHT_MEM_TRACKER_H
