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

//

#ifndef STARLIGHT_MPMC_QUEUE_H
#define STARLIGHT_MPMC_QUEUE_H

#include "base/thread/atomics.inl"

/*
LOCK FREE QUEUE IMPLEMENTATION FROM https://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
*/

#define QUEUE_CACHELINE_SIZE 64

#define MAKE_MPMC_QUEUE_TYPE(name, type) \
struct mpmc_queue_##name##_cell;\
typedef struct mpmc_queue_##name##_c\
{\
char pad0[QUEUE_CACHELINE_SIZE];\
struct mpmc_queue_##name##_cell* buffer;\
sl_atomic_uint64_t buffer_mask;\
char pad1[QUEUE_CACHELINE_SIZE];\
sl_atomic_uint64_t enqueue_pos;\
char pad2[QUEUE_CACHELINE_SIZE];\
sl_atomic_uint64_t dequeue_pos;\
char pad3[QUEUE_CACHELINE_SIZE]; \
} mpmc_queue_##name##_c;                                         \
typedef struct mpmc_queue_##name##_cell    \
{                                        \
sl_atomic_uint64_t sequence;                   \
type data; \
}mpmc_queue_##name##_cell;               \
static void mpmc_queue_##name##_init(mpmc_queue_##name##_c* queue, mpmc_queue_##name##_cell* cells, uint32_t cell_count) \
{\
queue->buffer = cells;\
queue->buffer_mask = cell_count - 1;\
\
for (uint64_t i = 0; i != cell_count; i+=1) {\
atomic_store_explicit(&queue->buffer[i].sequence, i, memory_order_relaxed);\
}                                        \
atomic_store_explicit(&queue->enqueue_pos, 0, memory_order_relaxed);\
atomic_store_explicit(&queue->dequeue_pos, 0, memory_order_relaxed);\
}                                        \
static void mpmc_queue_##name##_push(mpmc_queue_##name##_c* queue, type const *data)\
{\
bool found_cell = false;\
while (!found_cell) {\
mpmc_queue_##name##_cell *cell;\
uint64_t pos = atomic_load_explicit(&queue->enqueue_pos, memory_order_relaxed);\
for (;;) {\
cell = &queue->buffer[pos & queue->buffer_mask];\
const uint64_t seq = atomic_load_explicit(&cell->sequence, memory_order_acquire);\
const intptr_t dif = (intptr_t)seq - (intptr_t)pos;\
if (dif == 0) {\
if (atomic_compare_exchange_weak_explicit(&queue->enqueue_pos, &pos, pos+1, memory_order_relaxed, memory_order_relaxed) ) {\
found_cell = true;\
break;\
}\
} else if (dif < 0) {\
break;\
} else {\
pos = atomic_load_explicit(&queue->enqueue_pos, memory_order_relaxed);\
}\
}\
if (found_cell) {\
cell->data = *data;\
atomic_store_explicit(&cell->sequence, pos+1, memory_order_release);\
return;\
}\
}\
}                                        \
static int mpmc_queue_##name##_pop(mpmc_queue_##name##_c* queue, type* data)\
{\
mpmc_queue_##name##_cell *cell;\
uint64_t pos = atomic_load_explicit(&queue->dequeue_pos, memory_order_relaxed);\
for (;;) {\
cell = &queue->buffer[pos & queue->buffer_mask];\
const uint64_t seq = atomic_load_explicit(&cell->sequence, memory_order_acquire);\
const intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);\
if (dif == 0) {\
if (atomic_compare_exchange_weak_explicit(&queue->dequeue_pos, &pos, pos+1, memory_order_relaxed, memory_order_relaxed)) {\
break;\
}\
} else if (dif < 0) {\
return 0;\
} else {\
pos = atomic_load_explicit(&queue->dequeue_pos, memory_order_relaxed);\
}\
}\
\
*data = cell->data;\
atomic_store_explicit(&cell->sequence, pos+queue->buffer_mask+1, memory_order_release);\
return 1;\
}



#endif //STARLIGHT_MPMC_QUEUE_H
