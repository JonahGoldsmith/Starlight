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

#ifndef STARLIGHT_JOB_SYSTEM_H
#define STARLIGHT_JOB_SYSTEM_H

/*
Referenced https://ruby0x1.github.io/machinery_blog_archive/post/fiber-based-job-system/index.html
Referenced https://github.com/edubart/minicoro
Referenced https://github.com/krzysztofmarecki/JobSystem
Referenced https://github.com/Freeeaky/fiber-job-system
Referenced https://github.com/JodiTheTigger/sewing
Referenced https://github.com/SergeyMakeev/TaskScheduler
Referenced https://github.com/RichieSams/FiberTaskingLib
*/

#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sl_allocator;

/**
 * @brief Opaque Representation of an Atomic Counter
 * That Gets Incremented/Decremented by the Job System
 */
typedef struct sl_job_counter sl_job_counter;

/**
 * @brief Representation of a Job's Priority... How important it is.
 */
typedef enum sl_job_priority {
    sl_normal_priority = 0,
    sl_high_priority = 1
} sl_job_priority;

/**
 * @brief Representation of a Jobs Stack Size
 */
typedef enum sl_job_stack_size {
    sl_ss_normal = 0,
    sl_ss_extended = 1
} sl_job_stack_size;

/**
 * @brief Representation of a Job To Send To The Job System
 */
typedef struct sl_job_decl {

/**
 * @brief Function Pointer For This Job to Run
 * @param data Data to be given to the *task* function
 */
    void (*task)(void *data);

/**
 * @brief Data that will be given to *task* function
 */
    void *data;

/**
 * @brief Priority of the Job
 */
    sl_job_priority priority;
    //TODO: Might be Better to have RunJobs function say which priority the jobs are?

/**
 * @brief Thread ID that this job was pinned too
 * Only needs to be used in certain situations...
 */
    uint32_t pinned_index;

} sl_job_decl;

/**
 * @brief Interface of the Entire Job System API.
 */
struct sl_job_system_api {
/**
 * @brief Run a Certain Amount of Jobs, Given their Declarations
 * @param jobs Array of Job Declarations
 * @param num_jobs Number of Jobs to Run
 * @param stack_size Which Stack Size to Use for the Jobs
 * @returns An Atomic Counter, Decremented when each Job finishes
 */
    sl_job_counter *(*run_jobs)(sl_job_decl *jobs, uint32_t num_jobs, sl_job_stack_size stack_size);

/**
 * @brief Run a Certain Amount of Jobs, Given their Declarations, But Automatically Frees the Counter
 * @param jobs Array of Job Declarations
 * @param num_jobs Number of Jobs to Run
 * @param stack_size Which Stack Size to Use for the Jobs
 */
    void (*run_jobs_and_free)(sl_job_decl *jobs, uint32_t num_jobs, sl_job_stack_size stack_size);

/**
 * Waits for a Given Counter to reach a given Value...
 * Must Use 0 for Value to Wait for All Jobs to Finish
 * This Function Does not Free the Counter! This Makes it so that the User can Reuse the Counter!
 * @param counter Which Counter to Wait On
 * @param value The Value we Wait for the Counter to Equal
 */
    void (*wait_for_counter)(sl_job_counter *counter, uint32_t value);

/**
 * @brief Waits for a Given Counter to Reach 0 then Frees it.
 * @param counter Which Counter to Wait On
 */
    void (*wait_for_counter_free)(sl_job_counter *counter);

/**
 * @brief Works the Same as WaitForCounterAndFree But Can be Used Outside of Main Job System Thread.
 * @param counter Which Counter to Wait On
 * @param time Amount of Time to Sleep the Thread
 */
    void (*wait_for_counter_os)(sl_job_counter *counter, double time);
    //TODO: Is this ever used outside of the job system shutdown?

/**
 * @brief Used to Get a Thread ID From the Job System based on an index.
 * @param index the Thread index to be pinned too
 * @returns Internal Thread ID of Specified Index
 */
    uint32_t (*get_pin_index)(uint32_t index);

};

typedef struct sl_job_system_desc {
    /**
 * @brief Number of Threads to be Created by the Job System, Should be one Thread for Each CPU Core
 */
    uint32_t num_threads;
    /**
     * @pbrief Number of Fibers in the Job System. Must be a power of 2. MAX IS 256
     */
    uint32_t num_fibers;
    /**
 * @pbrief Stack Size for Normal Fibers
 */
    uint32_t normal_stack_size;
    /**
* @pbrief Extended Stack Size for BIG Fibers
*/
    uint32_t extended_stack_size;
    /**
* @pbrief Extended Stack Size for BIG Fibers
*/
    struct sl_allocator *p_allocator;
} sl_job_system_desc;

#define SL_JOB_SYSTEM_API "sl_job_system_api"

#ifdef LINKS_SL_BASE

/**
 * @brief Creates the Job System
 * @param p_desc Pointer to Local sl_job_system_desc Struct Used for Initialization
 * @returns The Created sl_job_system_api
 */
extern struct sl_job_system_api *sl_create_job_system(sl_job_system_desc *p_desc);

/**
 * @brief Destroys the Job System
 */
extern void sl_destroy_job_system(void);

/**
 * @brief Retrives the Global Job System Api
 * @returns The Created sl_job_system_api struct
 */
extern struct sl_job_system_api *sl_get_job_system(void);

#endif

#ifdef __cplusplus
}
#endif

#endif //STARLIGHT_JOB_SYSTEM_H
