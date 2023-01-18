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


/*
Referenced https://ruby0x1.github.io/machinery_blog_archive/post/fiber-based-job-system/index.html
Referenced https://github.com/edubart/minicoro
Referenced https://github.com/krzysztofmarecki/job_system
Referenced https://github.com/Freeeaky/fiber-job-system
Referenced https://github.com/JodiTheTigger/sewing
Referenced https://github.com/SergeyMakeev/TaskScheduler
Referenced https://github.com/RichieSams/FiberTaskingLib
*/

#include "job_system.h"
#include "base/util/sprintf.h"
#include "base/os/os.h"
#include "base/data_structures/mpmc_queue.h"
#include "memory/allocator.h"
#include "data_structures/hash.inl"
#include <stdio.h>


extern struct sl_os_api* sl_os_api; //Found in os_activeplatform.c
extern struct sl_sprintf_api* sl_sprintf_api; //Found in sprintf.c

//Hashmap
typedef struct semaphore_map
{
    uint64_t key;
    uint32_t value;
}semaphore_map;

struct sl_job_counter
{
    uint32_t counter_index;
    sl_atomic_uint32_t counter;
    sl_job_stack_size stack_size;
};

//Internal Representation of a Job in the Job System
typedef struct internal_job
{
    sl_job_decl job_decl;
    sl_job_counter* counter;
    // If true the counter is automatically freed
    bool auto_free;
} internal_job;

struct job_fiber;

//Representation of a waiting_fiber to be put in the waiting queue
typedef struct waiting_fiber
{
    uint32_t counter_condition;
    uint32_t counter_index;
    struct job_fiber* fiber;
} waiting_fiber;

//Internal Representation of a fiber that includes the next/waiting fiber and the pinned thread handle if supplied
typedef struct job_fiber
{
    uint32_t pinned_index;
    uint32_t fiber_index;
    sl_os_fiber fiber_id;
    waiting_fiber wait_fiber;
    sl_job_stack_size stack_size;
} job_fiber;


#define MAX_WORKER_THREADS 128
#define MAX_FIBERS 256
#define MAX_JOBS 4096

MAKE_MPMC_QUEUE_TYPE(uint32, uint32_t)
MAKE_MPMC_QUEUE_TYPE(job, internal_job)
MAKE_MPMC_QUEUE_TYPE(wait, waiting_fiber)

/*
Internal Job System Representation...
Allocates all memory and fibers upfront so no dynamic memory allocation
*/
typedef struct internal_job_system
{
    //Used for Initialization
    sl_os_thread_api *thread_api;
    sl_atomic_bool running;

    //Threads and Fibers
    uint32_t num_worker_threads;
    sl_os_thread worker_threads[MAX_WORKER_THREADS];
    uint32_t worker_thread_ids[MAX_WORKER_THREADS];
    uint32_t num_fibers;
    job_fiber fibers[MAX_FIBERS];
    sl_job_counter counters[MAX_JOBS];

    //Semaphores To Wake Threads
    sl_os_semaphore semaphores[MAX_WORKER_THREADS];
    semaphore_map* thread_semaphores;
    sl_atomic_uint32_t next_wakeup;

    struct sl_allocator* p_allocator;

    //MPMC Queues
    mpmc_queue_uint32_c free_normal_indices;
    mpmc_queue_uint32_c free_extended_indices;
    mpmc_queue_uint32_c free_counters;
    mpmc_queue_wait_c wait_queue;
    mpmc_queue_job_c priority_queue;
    mpmc_queue_job_c normal_queue;

} internal_job_system;

//This is our job system! //TODO: Switch to allocating memory client side and passing it in so we dont store this huge struct on the stack
static internal_job_system job_system;

static void job_proc(void *params)
{
    //If the job system is not active yield the thread.
    while (!job_system.running)
        job_system.thread_api->thread_yield();

    //Next Waiting Fiber From Queue
    waiting_fiber wait_fiber;
    //Current Job to Run.
    internal_job job;

    //Loop over the queues while the job system is active...
    //Each Worker Thread Will Run This Loop Separately
    while (job_system.running) {
        job_fiber *f = (job_fiber *)job_system.thread_api->get_fiber_data();
        if (f->wait_fiber.fiber) {
            mpmc_queue_wait_push(&job_system.wait_queue, &f->wait_fiber);
            f->wait_fiber.fiber = NULL;
        }

        const bool waiting_fibers = mpmc_queue_wait_pop(&job_system.wait_queue, &wait_fiber);
        //Check to see if we have any waiting fibers we can resume...
        if (waiting_fibers) {
            //If we have one check to see if the condition is met
            if (atomic_load_explicit(&job_system.counters[wait_fiber.counter_index].counter, memory_order_acquire) == wait_fiber.counter_condition) {
                //If the condition is met only accept the fiber if we are the thread it is pinned too
                bool resume = wait_fiber.fiber->pinned_index == 0 || wait_fiber.fiber->pinned_index == job_system.thread_api->get_thread_id();
                if (resume) {
                    //If we are resuming we must return the fiber to the pool of free fibers
                    job_fiber *cur_fiber = (job_fiber *)job_system.thread_api->get_fiber_data();

                    switch(cur_fiber->stack_size)
                    {
                        case sl_ss_normal:
                            //TODO: Should the new fiber return the old fiber to the free fiber list ?
                            mpmc_queue_uint32_pop(&job_system.free_normal_indices, &cur_fiber->fiber_index);
                            break;
                        case sl_ss_extended:
                            //TODO: Should the new fiber return the old fiber to the free fiber list ?
                            mpmc_queue_uint32_pop(&job_system.free_extended_indices, &cur_fiber->fiber_index);
                            break;
                    }

                    job_system.thread_api->switch_to_fiber(wait_fiber.fiber->fiber_id);
                    continue;
                } else {
                    //If the waiting fiber was pinned to a thread, and we aren't that thread we must put it back in the queue.
                    mpmc_queue_wait_push(&job_system.wait_queue, &wait_fiber);
                    //We wake up whichever thread the job was pinned too...
                    const uint64_t key = wait_fiber.fiber->pinned_index;
                    job_system.thread_api->add_semaphore_count(job_system.semaphores[sl_hashmap_get(job_system.p_allocator, job_system.thread_semaphores, key)], 1);

                }
            } else { //Condition Was not met
                //Wake up the pinned thread while waiting for the condition to be met
                if (wait_fiber.fiber->pinned_index != 0) {
                    const uint64_t key = wait_fiber.fiber->pinned_index;
                    job_system.thread_api->add_semaphore_count(job_system.semaphores[sl_hashmap_get(job_system.p_allocator, job_system.thread_semaphores, key)], 1);
                }
                //Put the fiber back in the queue so that the pinned thread can find it.
                mpmc_queue_wait_push(&job_system.wait_queue, &wait_fiber);
            }
        }

        //Always Check For Priority Jobs First!
        if (mpmc_queue_job_pop(&job_system.priority_queue, &job))
        {
            //Check to see if the job was pinned to a certain thread
            bool accept_job = job.job_decl.pinned_index == 0
                             || job.job_decl.pinned_index == job_system.thread_api->get_thread_id();
            if (accept_job) {
                f->pinned_index = job.job_decl.pinned_index;
                //run the job if not NULL.
                if (job.job_decl.task)
                    job.job_decl.task(job.job_decl.data);

                if (job.job_decl.pinned_index != 0) {
                    job_fiber *cur_fiber = (job_fiber *)job_system.thread_api->get_fiber_data();
                    //Unpin the fiber from the thread
                    cur_fiber->pinned_index = 0;
                }

                //Decrement the job counter after running the job
                atomic_fetch_sub(&job.counter->counter, 1);
                if (job.auto_free && job.counter->counter == 0) {
                    mpmc_queue_uint32_push(&job_system.free_counters, &job.counter->counter_index);
                }
            } else {
                //Job was pinned to a thread, but we aren't it... so put it back on the queue
                mpmc_queue_job_push(&job_system.priority_queue, &job);
                //Wake up the pinned thread
                const uint64_t key = job.job_decl.pinned_index;
                job_system.thread_api->add_semaphore_count(job_system.semaphores[sl_hashmap_get(job_system.p_allocator, job_system.thread_semaphores, key)], 1);
            }
        } else if (mpmc_queue_job_pop(&job_system.normal_queue, &job)) {  //Now Check For the Normal Priority Queue
            bool accept_job = job.job_decl.pinned_index == 0
                             || job.job_decl.pinned_index == job_system.thread_api->get_thread_id();
            if (accept_job) {
                f->pinned_index = job.job_decl.pinned_index;
                if (job.job_decl.task)
                    job.job_decl.task(job.job_decl.data);

                if (job.job_decl.pinned_index != 0) {
                    job_fiber *cur_fiber = (job_fiber *)job_system.thread_api->get_fiber_data();
                    cur_fiber->pinned_index = 0;
                }
                atomic_fetch_sub(&job.counter->counter, 1);
                if (job.auto_free && job.counter->counter == 0)
                    mpmc_queue_uint32_push(&job_system.free_counters, &job.counter->counter_index);
            } else {
                mpmc_queue_job_push(&job_system.normal_queue, &job);
                const uint64_t key = job.job_decl.pinned_index;
                job_system.thread_api->add_semaphore_count(job_system.semaphores[sl_hashmap_get(job_system.p_allocator, job_system.thread_semaphores, key)], 1);
            }
        } else if (!waiting_fibers) { //If no jobs are in the priority or normal queue, and we don't have any waiting fibers...
            //Wait on the running thread until we do have jobs to run.
            const uint64_t key = job_system.thread_api->get_thread_id();
            job_system.thread_api->wait_semaphore(job_system.semaphores[sl_hashmap_get(job_system.p_allocator,job_system.thread_semaphores, key)]);

        }
    }//Job system no longer running


    //We must make sure the fibers that were converted from threads are destroyed by calling fiber to thread on them instead of destroy
    uint32_t thread_id = job_system.thread_api->get_thread_id();
    uint32_t thread_index = 0;
    for (uint32_t i = 0; i != job_system.num_worker_threads; i++) {
        if (job_system.worker_thread_ids[i] == thread_id) {
			thread_index = i;
            break;
        }
    }
    const job_fiber *f = (job_fiber *)job_system.thread_api->get_fiber_data();
    if (f != &job_system.fibers[thread_index])
        job_system.thread_api->switch_to_fiber(job_system.fibers[thread_index].fiber_id);

    job_system.thread_api->fiber_to_thread();
}

//Data For The Worker Threads That Will Run Jobs
struct job_worker_thread
{
    uint32_t worker_thread_index;
    sl_atomic_uint32_t* wake_counter;
};

//Entry/Start Function for Our Worker Threads...
//Each Worker Thread Will Run the job_proc loop over waiting fiber queue, priority queue and normal queue
static void start_worker_thread(void*user_data)
{
    struct job_worker_thread *thread_data = (struct job_worker_thread *)user_data;
    uint64_t worker_id = thread_data->worker_thread_index;
    char name[128];
    sl_sprintf(name, "Job Worker: %llu", worker_id);
    sl_os_api->thread->set_thread_name(name);
    job_system.worker_thread_ids[worker_id] = job_system.thread_api->get_thread_id();
    job_fiber *f = job_system.fibers + worker_id;
    f->wait_fiber.fiber = NULL;
    f->fiber_index = (uint32_t)worker_id;
    f->fiber_id = job_system.thread_api->thread_to_fiber(&job_system.fibers[worker_id]);
    atomic_fetch_sub(thread_data->wake_counter, 1);

    job_proc(&job_system.fibers[worker_id]);
}

static void run_jobs_and_free(sl_job_decl *jobs, uint32_t num_jobs, sl_job_stack_size stack_size)
{
    uint32_t free_counter_index;
    //Spin on the counters queue until we get a free counter
    while (!mpmc_queue_uint32_pop(&job_system.free_counters, &free_counter_index))
    {

    }
    sl_job_counter *counter = &job_system.counters[free_counter_index];

    internal_job j = {};
    j.auto_free = true;

    atomic_store(&counter->counter, num_jobs);
    j.counter = counter;
    for (uint32_t i = 0; i != num_jobs; ++i) {
        j.job_decl = jobs[i];
        if(jobs[i].priority == sl_normal_priority) // If Normal, add to Normal Queue
        {
            mpmc_queue_job_push(&job_system.normal_queue, &j);
        }else if(jobs[i].priority == sl_high_priority) //If High, add to Priority Queue
        {
            mpmc_queue_job_push(&job_system.priority_queue, &j);
        }
        if (jobs[i].pinned_index) {
            const uint64_t key = jobs[i].pinned_index;
            job_system.thread_api->add_semaphore_count(job_system.semaphores[sl_hashmap_get(job_system.p_allocator, job_system.thread_semaphores, key)], 1);
        } else {
            uint32_t index = atomic_fetch_add(&job_system.next_wakeup, 1);
            const uint64_t key = job_system.thread_api->get_thread_id();
            if ((index % job_system.num_worker_threads) == sl_hashmap_get(job_system.p_allocator, job_system.thread_semaphores, key))
                index = atomic_fetch_add(&job_system.next_wakeup, 1);
            job_system.thread_api->add_semaphore_count(job_system.semaphores[index % job_system.num_worker_threads], 1);
        }
    }

}

static sl_job_counter *run_jobs(sl_job_decl *jobs, uint32_t num_jobs, sl_job_stack_size stack_size)
{
    uint32_t freecounter_index;
    //Spin on the counters queue until we get a free counter
    while (!mpmc_queue_uint32_pop(&job_system.free_counters, &freecounter_index))
    {

    }
    sl_job_counter *counter = &job_system.counters[freecounter_index];

    internal_job j = {};
    j.auto_free = false;

    atomic_store(&counter->counter, num_jobs);
    j.counter = counter;
    for (uint32_t i = 0; i != num_jobs; ++i) {
        j.job_decl = jobs[i];
        if(jobs[i].priority == sl_normal_priority) // If Normal, add to Normal Queue
        {
            mpmc_queue_job_push(&job_system.normal_queue, &j);
        }else if(jobs[i].priority == sl_high_priority) //If High, add to Priority Queue
        {
            mpmc_queue_job_push(&job_system.priority_queue, &j);
        }
        if (jobs[i].pinned_index) {
            const uint64_t key = jobs[i].pinned_index;
            job_system.thread_api->add_semaphore_count(job_system.semaphores[sl_hashmap_get(job_system.p_allocator, job_system.thread_semaphores, key)], 1);
        } else {
            uint32_t index = atomic_fetch_add(&job_system.next_wakeup, 1);
            const uint64_t key = job_system.thread_api->get_thread_id();
            if ((index % job_system.num_worker_threads) == sl_hashmap_get(job_system.p_allocator, job_system.thread_semaphores, key))
                index = atomic_fetch_add(&job_system.next_wakeup, 1);
            job_system.thread_api->add_semaphore_count(job_system.semaphores[index % job_system.num_worker_threads], 1);
        }
    }
    return counter;
}

static void wait_for_counter(sl_job_counter *c, uint32_t value)
{
    if (atomic_load_explicit(&c->counter, memory_order_acquire) != value) {
        uint32_t free_fiber_index;
        //Spin lock until we get a free fiber
        switch(c->stack_size)
        {
            case sl_ss_normal:
                while (!mpmc_queue_uint32_pop(&job_system.free_normal_indices, &free_fiber_index))
                {

                }
                break;
            case sl_ss_extended:
                while (!mpmc_queue_uint32_pop(&job_system.free_extended_indices, &free_fiber_index))
                {

                }
                break;
        }

        job_fiber *next_fiber = job_system.fibers + free_fiber_index;

        //Put fiber to wait after using it
        job_fiber *cur_fiber = (job_fiber *)job_system.thread_api->get_fiber_data();

        waiting_fiber waiting_fiber = {value, c->counter_index, cur_fiber };

        next_fiber->wait_fiber = waiting_fiber;
        job_system.thread_api->switch_to_fiber(next_fiber->fiber_id);
    }
}

static void wait_and_free(sl_job_counter *c)
{
	wait_for_counter(c, 0);

    //put the counter back on the queue
    mpmc_queue_uint32_push(&job_system.free_counters, &c->counter_index);
}

static void wait_and_free_os(sl_job_counter *c, double sleep)
{

    //TODO: Sleep V Yield?

    if (sleep) {
        while (atomic_load_explicit(&c->counter, memory_order_acquire) != 0)
        {
            job_system.thread_api->sleep(sleep);
        }
    } else {
        while (atomic_load_explicit(&c->counter, memory_order_acquire) != 0)
        {

        }
    }

    //Put the counter back into the queue
    mpmc_queue_uint32_push(&job_system.free_counters, &c->counter_index);
}

static uint32_t get_pin_index(uint32_t index)
{
    return job_system.worker_thread_ids[index];
}

static struct sl_job_system_api sl_job_system_api = {
	run_jobs,
	run_jobs_and_free,
	wait_for_counter,
	wait_and_free,
	wait_and_free_os,
	get_pin_index,
};

//Queue Cells (Used for our C Based MPMC Queue)
static mpmc_queue_uint32_cell* free_normal_cells;
static mpmc_queue_uint32_cell* free_extended_cells;
static mpmc_queue_uint32_cell* free_counter_cells;
static mpmc_queue_job_cell* normal_queue_cells;
static mpmc_queue_job_cell* priority_queue_cells;
static mpmc_queue_wait_cell* wait_queue_cells;

struct sl_job_system_api *sl_create_job_system(sl_job_system_desc* p_desc)
{
    job_system.p_allocator = p_desc->p_allocator;
    //Queue Cells (Used for our C Based MPMC Queue)
    free_normal_cells = (mpmc_queue_uint32_cell*)sl_alloc(p_desc->p_allocator, sizeof(mpmc_queue_uint32_cell) * MAX_FIBERS);
    free_extended_cells = (mpmc_queue_uint32_cell*)sl_alloc(p_desc->p_allocator, sizeof(mpmc_queue_uint32_cell) * MAX_FIBERS);
    free_counter_cells = (mpmc_queue_uint32_cell*)sl_alloc(p_desc->p_allocator, sizeof(mpmc_queue_uint32_cell) * MAX_JOBS);

    normal_queue_cells = (mpmc_queue_job_cell*)sl_alloc(p_desc->p_allocator, sizeof(mpmc_queue_job_cell) * MAX_JOBS);
    priority_queue_cells = (mpmc_queue_job_cell*)sl_alloc(p_desc->p_allocator, sizeof(mpmc_queue_job_cell) * MAX_JOBS);

    wait_queue_cells = (mpmc_queue_wait_cell*)sl_alloc(p_desc->p_allocator, sizeof(mpmc_queue_wait_cell) * MAX_FIBERS);

    job_system.thread_semaphores = NULL;
    job_system.thread_api = sl_os_api->thread;
    job_system.num_fibers = p_desc->num_fibers;
    mpmc_queue_wait_init(&job_system.wait_queue, wait_queue_cells, p_desc->num_fibers);
    mpmc_queue_uint32_init(&job_system.free_normal_indices, free_normal_cells, p_desc->num_fibers);
    mpmc_queue_uint32_init(&job_system.free_extended_indices, free_extended_cells, 24);

    sl_atomic_uint32_t wake_counter;
    atomic_store(&wake_counter, p_desc->num_threads);
    struct job_worker_thread wtd[MAX_WORKER_THREADS];
    for (uint32_t i = 0; i != p_desc->num_threads; ++i) {
        wtd[i].wake_counter = &wake_counter;
        wtd[i].worker_thread_index = i;
        char debug_name[128];
        sl_sprintf(debug_name, "Job System: Thread %d", i);
        job_system.worker_threads[i] = sl_os_api->thread->create_os_thread(start_worker_thread, &wtd[i], 0, debug_name);
        sl_os_api->thread->set_thread_affinity(job_system.worker_threads[i], i);
        job_system.semaphores[i] = sl_os_api->thread->init_semaphore(0);
        const uint64_t key = sl_os_api->thread->get_thread_id_from_thread(job_system.worker_threads[i]);
        sl_hashmap_push(job_system.p_allocator, job_system.thread_semaphores, key, i);

    }

    //Wake up all threads and wait for them to be awake
    while (atomic_load_explicit(&wake_counter, memory_order_acquire)!= 0) {
        job_system.thread_api->sleep(0.01);
    }
    job_system.num_worker_threads = p_desc->num_threads;

    mpmc_queue_uint32_init(&job_system.free_counters, free_counter_cells, MAX_JOBS);

    mpmc_queue_job_init(&job_system.normal_queue, normal_queue_cells, MAX_JOBS);

    mpmc_queue_job_init(&job_system.priority_queue, priority_queue_cells, MAX_JOBS);

    for (uint32_t i = 0; i != MAX_JOBS; ++i) {
        job_system.counters[i].counter_index = i;
        mpmc_queue_uint32_push(&job_system.free_counters, &i);
    }

    job_fiber f = { 0 };
    uint32_t index = p_desc->num_threads;
    for (uint32_t i = p_desc->num_threads; i != (p_desc->num_fibers-8); i++) {
        f.fiber_index = i;
        f.stack_size = sl_ss_normal;
        f.fiber_id = job_system.thread_api->create_fiber(job_proc, &job_system.fibers[i], p_desc->normal_stack_size);
        job_system.fibers[i] = f;
        mpmc_queue_uint32_push(&job_system.free_normal_indices, &i);
        index++;
    }

    for (uint32_t i = 0; i != 8; i++) {
        f.fiber_index = index;
        f.stack_size = sl_ss_extended;
        f.fiber_id = job_system.thread_api->create_fiber(job_proc, &job_system.fibers[index], p_desc->extended_stack_size);
        job_system.fibers[index] = f;
        mpmc_queue_uint32_push(&job_system.free_extended_indices, &index);
        index++;
    }

    atomic_store_explicit(&job_system.running, true, memory_order_release);
    return &sl_job_system_api;
}

void sl_destroy_job_system()
{

    //Lets the OS close down all Threads... Then destroy fibers

    //Exit the Job System
    atomic_store_explicit(&job_system.running, false, memory_order_release);

    sl_os_thread_api *thread_api = job_system.thread_api;

    //Destroy all fibers that were made with CreateFiber
    for (uint32_t i = job_system.num_worker_threads; i != job_system.num_fibers; i++) {
        thread_api->destroy_fiber(job_system.fibers[i].fiber_id);
    }

    for(uint32_t i = 0; i != job_system.num_worker_threads; i++)
    {
        thread_api->destroy_fiber(job_system.fibers[i].fiber_id);
    }

    //Queue Cells (Used for our C Based MPMC Queue)
    sl_free(job_system.p_allocator, free_normal_cells);
	sl_free(job_system.p_allocator, free_extended_cells);
	sl_free(job_system.p_allocator, free_counter_cells);
	sl_free(job_system.p_allocator, normal_queue_cells);
	sl_free(job_system.p_allocator, priority_queue_cells);
	sl_free(job_system.p_allocator, wait_queue_cells);

    sl_hashmap_free(job_system.p_allocator, job_system.thread_semaphores);

    job_system.thread_api = NULL;

}

struct sl_job_system_api *sl_get_job_system(void)
{
    return job_system.thread_api ? &sl_job_system_api : NULL;
}

