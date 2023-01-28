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
// Created by Jonah Goldsmith on 1/3/23.
//
#include "os.h"
#ifdef SL_PLATFORM_OSL

#include "base/thread/atomics.inl"
#include "memory/allocator.h"

extern struct sl_allocator_api* sl_allocator_api;

#include <unistd.h>

#pragma region Semaphores

#include <mach/mach.h>
#include <mach/semaphore.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>

static sl_os_semaphore macos_create_semaphore(uint32_t initial_count)
{
    semaphore_t out;
    kern_return_t res = semaphore_create(mach_task_self(), &out, SYNC_POLICY_FIFO, (int)initial_count);
    sl_os_semaphore sem = {0};
    memcpy(&sem, &out, sizeof(out));
    return sem;
}

static void macos_add_semaphore(sl_os_semaphore sem, uint32_t value)
{
    semaphore_t out;
    memcpy(&out, &sem, sizeof(out));
    for (uint32_t i = 0; i < value; ++i) {
        kern_return_t res = semaphore_signal(out);
    }
}

static void macos_close_semaphore(sl_os_semaphore sem)
{
    semaphore_t out;
    memcpy(&out, &sem, sizeof(out));
    kern_return_t res = semaphore_destroy(mach_task_self(), out);
}

static void macos_wait_semaphore(sl_os_semaphore sem)
{
    semaphore_t out;
    memcpy(&out, &sem, sizeof(out));
    kern_return_t res = semaphore_wait(out);
}

#pragma endregion

#pragma region Threads

//Internal Representation of Data needed for a Thread
struct internal_thread_data
{
    thread_entry *entry;
    void *user_data;
    const char* debug_name;
};


static void* internal_thread_func(void *data)
{
    struct internal_thread_data *itd = (struct internal_thread_data *)data;
    struct internal_thread_data td = *itd;
    sl_free(sl_allocator_api->system, itd);
    td.entry(td.user_data);
    if (td.debug_name)
        pthread_setname_np(td.debug_name);
    return NULL;
}

static sl_os_thread macos_create_thread(thread_entry *entry, void *user_data, uint32_t stack_size, const char *debug_name)
{
    struct internal_thread_data *td = sl_alloc(sl_allocator_api->system, sizeof(struct internal_thread_data));

    td->entry = entry;
    td->user_data = user_data;
    td->debug_name = debug_name;

    pthread_t pthread;
    int res = pthread_create(&pthread, NULL, internal_thread_func, td);
    //assert(res == 0);

    sl_os_thread thread = { 0 };
    memcpy(&thread, &pthread, sizeof(pthread));
    return thread;
}

static void macos_set_thread_name(const char* name)
{
    pthread_setname_np(name);
}

static void macos_get_thread_name(char* buffer, int size)
{
    pthread_getname_np(pthread_self(), buffer, size);
}

SL_FORCE_INLINE uint32_t hash_uint64(uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccd;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53;
    x ^= x >> 33;
    return (uint32_t)x;
}

static uint32_t macos_thread_id_from_thread(sl_os_thread thread)
{
    return hash_uint64(thread.internal[0]);
}

static SL_THREAD_LOCAL bool thread_id_init = false;
static SL_THREAD_LOCAL uint32_t local_thread_id = { 0 };

static uint32_t macos_get_thread_id(void)
{
    if (!thread_id_init) {
        pthread_t self = pthread_self();
        uint64_t id;
        sl_memcpy(&id, &self, sizeof(uint64_t));
        local_thread_id = hash_uint64(id);
        thread_id_init = true;
    }
    return local_thread_id;
}

static void macos_set_thread_affinity(sl_os_thread thread, uint32_t value)
{
    //Cant Set Affinity on MACOS... Do nothing...
}

#pragma endregion

/*
Referenced https://ruby0x1.github.io/machinery_blog_archive/post/fiber-based-job-system/index.html
Referenced https://github.com/edubart/minicoro
Referenced https://github.com/krzysztofmarecki/JobSystem
Referenced https://github.com/Freeeaky/fiber-job-system
Referenced https://github.com/JodiTheTigger/sewing
*/
#pragma region Fibers

#define MINICORO_IMPL
#define MCO_NO_DEFAULT_ALLOCATORS
#include "third-party/minicoro/minicoro.h"

//Max Amount of Fibers Allowed to be Created!
#define MAX_FIBERS 2048

//Internal Representation of a Fiber
struct internal_fiber_data
{
    mco_coro* context;
    mco_desc desc;
    fiber_entry* entry;
    void* user_data;
    uint32_t stack_size;
};

//Array of Fibers
static struct internal_fiber_data fibers[MAX_FIBERS];
//Fiber Count
static uint32_t num_fibers = 1;

//Fiber Index used to find the fiber in the array of fibers...
//Must be thread local so that each thread can access a local copy
static SL_THREAD_LOCAL uint32_t fib_index;

//Main Fiber Index for keeping track of the main Fiber running the application.
//Must be thread local so that each thread can access a local copy
static SL_THREAD_LOCAL uint32_t main_fib_index;

//Use this to see if we have initialized a fiber...
// TODO: its only use is to initialize a Critical Section... Should this be changed?
bool initialized = false;
//Critical Section Only Used by Fiber Creation
pthread_mutex_t fiber_mutex = PTHREAD_MUTEX_INITIALIZER;
/*
Getter/Setter For Fiber Index and Main Fiber Index
*/
static uint32_t get_fib_index(void)
{
    return fib_index;
}

static void set_fib_index(uint32_t i)
{
    fib_index = i;
}

static uint32_t get_main_fib_index(void)
{
    return main_fib_index;
}

static void set_main_fib_index(uint32_t i)
{
    main_fib_index = i;
}

// TODO: Use the Data passed from Minicoro?
static void* mini_coro_alloc(size_t size, void* data)
{
    return sl_alloc(sl_allocator_api->system, size);
}

static void mini_coro_free(void* ptr, void* data)
{
    sl_free(sl_allocator_api->system, ptr);
}

//Entry Point For Fibers... We use this isntead of the actual function passed to user_data
//Because minicoro requires us to yield after execution.
static void fiber_entry_point(mco_coro *context)
{
    struct internal_fiber_data *fiber = (struct internal_fiber_data*)context->user_data;
    if (fiber->entry)
        fiber->entry(fiber->user_data);
    mco_yield(context);
}

static sl_os_fiber macos_thread_to_fiber(void *user_data)
{

    // TODO: MCORO REQUIRES LOCKING BEFORE USE... IS THIS A PERFORMANCE KILLER OR BARELY NOTICED?
    if(!initialized) {
        pthread_mutex_init(&fiber_mutex, NULL);
        initialized = true;
    }

    pthread_mutex_lock(&fiber_mutex);
    uint32_t i = num_fibers++;
    struct internal_fiber_data *fiber = fibers + i;
    memset(fiber, 0, sizeof(*fiber));
    fiber->user_data = user_data;
    mco_desc fiber_desc = mco_desc_init(fiber_entry_point, 0);
    fiber_desc.malloc_cb = mini_coro_alloc;
    fiber_desc.free_cb = mini_coro_free;
    fiber_desc.user_data = fiber;
    fiber_desc.allocator_data = &fiber->stack_size;
    fiber->desc = fiber_desc;
    fiber->context = mini_coro_alloc(fiber_desc.coro_size, 0);
    mco_init(fiber->context, &fiber_desc);
    set_fib_index(i);
    set_main_fib_index(i);
    pthread_mutex_unlock(&fiber_mutex);

    sl_os_fiber out_fiber = { i };
    return out_fiber;

}

static void macos_switch_to_fiber(sl_os_fiber fiber);

static void macos_destroy_fiber(sl_os_fiber in_fiber)
{

    struct internal_fiber_data *fiber = fibers + in_fiber.internal;
    mco_uninit(fiber->context);
    mco_destroy(fiber->context);
}

//Memory Gets Cleaned Up By the User!!
static void macos_fiber_to_thread(void)
{
    if (get_main_fib_index() != get_fib_index()) {
        sl_os_fiber main = { get_main_fib_index() };
        macos_switch_to_fiber(main);
    }
}

static sl_os_fiber macos_create_fiber(fiber_entry *entry, void *user_data, uint32_t stack_size)
{

    // TODO: WOULD LOCK FREE WORK SINCE JOB SYSTEM SPAWNS ALL FIBERS UPFRONT?
    pthread_mutex_lock(&fiber_mutex);
    uint32_t i = num_fibers++;
    struct internal_fiber_data *fiber = fibers + i;
    memset(fiber, 0, sizeof(*fiber));
    fiber->entry = entry;
    fiber->user_data = user_data;
    fiber->stack_size = stack_size;
    mco_desc fiber_desc = mco_desc_init(fiber_entry_point, stack_size);
    fiber_desc.malloc_cb = mini_coro_alloc;
    fiber_desc.free_cb = mini_coro_free;
    fiber_desc.user_data = fiber;
    fiber_desc.allocator_data = &fiber->stack_size;
    fiber->desc = fiber_desc;
    fiber->context = mini_coro_alloc(fiber_desc.coro_size, 0);
    mco_init(fiber->context, &fiber_desc);
    pthread_mutex_unlock(&fiber_mutex);

    sl_os_fiber out_fiber = { i };
    return out_fiber;

}

//This function is how minicoro calls a switch between fibers.
static void macos_switch_to_fiber(sl_os_fiber fiber)
{

    uint32_t cur_fiber = get_fib_index();
    uint32_t target_fiber = (uint32_t)fiber.internal;
    struct internal_fiber_data *current = fibers + cur_fiber;
    struct internal_fiber_data *target = fibers + target_fiber;
    set_fib_index(target_fiber);
    _mco_context *current_context = (_mco_context *)current->context->context;
    _mco_context *target_context = (_mco_context *)target->context->context;
    _mco_switch(&current_context->ctx, &target_context->ctx);

}

static void *macos_fiber_data(void)
{

    struct internal_fiber_data *current = fibers + get_fib_index();
    return current->user_data;

}


static void macos_yield_processor(void)
{
    sched_yield();
}


static void macos_sleep(double seconds)
{
    usleep((uint32_t)(seconds * 1e6 + 0.5));
}

#pragma endregion

#include <sys/sysctl.h>

static uint32_t macos_logical_cores()
{
	//TODO: Cocoa API for logical cores
    int cores;
    size_t len = sizeof(cores);
    int result = sysctlbyname("hw.logicalcpu", &cores, &len, NULL, 0);
    return cores;
}

static sl_os_info_api macos_info = {
        .num_logical_cores = macos_logical_cores
};


static sl_os_thread_api macos_thread_api = {
        .create_fiber = macos_create_fiber,
        .destroy_fiber = macos_destroy_fiber,
        .get_fiber_data = macos_fiber_data,
        .fiber_to_thread = macos_fiber_to_thread,
        .switch_to_fiber = macos_switch_to_fiber,
        .thread_to_fiber = macos_thread_to_fiber,
        .thread_yield = macos_yield_processor,
        .sleep = macos_sleep,
        .init_semaphore = macos_create_semaphore,
        .add_semaphore_count = macos_add_semaphore,
        .close_semaphore = macos_close_semaphore,
        .wait_semaphore = macos_wait_semaphore,
        .get_thread_id = macos_get_thread_id,
        .create_os_thread = macos_create_thread,
        .get_thread_id_from_thread = macos_thread_id_from_thread,
        .set_thread_affinity = macos_set_thread_affinity,
        .get_thread_name = macos_get_thread_name,
        .set_thread_name = macos_set_thread_name,
};

static sl_os_filesystem_api macos_file_system = {
        .open_file_append = NULL,
        .open_file_read = NULL,
        .open_file_write = NULL,
        .file_close = NULL,
        .file_write = NULL,
};

struct sl_os_api *sl_os_api = &(struct sl_os_api){
        .thread = &macos_thread_api,
        .file_system = &macos_file_system,
        .info = &macos_info,
};

#endif
