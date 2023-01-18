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

#ifndef STARLIGHT_ALLOCATOR_H
#define STARLIGHT_ALLOCATOR_H

#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sl_allocator
{
    void* inst;

    uint32_t context;

    void *(*realloc)(struct sl_allocator *a, void *ptr, size_t new_size, uint32_t align, const char* func,
                     const char *file, uint32_t line);
}sl_allocator;

    typedef struct sl_allocator_statistics
    {
        SL_ATOMIC uint32_t total_allocation_count;

        SL_ATOMIC uint64_t total_amount_allocated;

    }sl_allocator_statistics;

struct sl_allocator_api
{
    sl_allocator* system;

    sl_allocator_statistics* stats;

	sl_allocator (*create_child)(const sl_allocator *parent, const char *name);

	void (*destroy_child)(const sl_allocator *child);

};

#define sl_alloc(a, size) (a)->realloc(a, 0, size, 0, SL_FUNCTION, __FILE__, __LINE__)

#define sl_free(a, ptr) (a)->realloc(a, ptr, 0, 0, SL_FUNCTION, __FILE__, __LINE__)

#define sl_realloc(a, ptr, size) (a)->realloc(a, ptr, size, 0, SL_FUNCTION, __FILE__, __LINE__)


#define SL_ALLOCATOR_API "sl_allocator_api"

#ifdef LINKS_SL_BASE
extern struct sl_allocator_api* sl_allocator_api;
#endif

#ifdef __cplusplus
}
#endif

#endif //STARLIGHT_ALLOCATOR_H
