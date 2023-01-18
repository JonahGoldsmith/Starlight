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

#ifndef STARLIGHT_OS_H
#define STARLIGHT_OS_H

#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Represents an OS Semaphore
 */
typedef struct sl_os_semaphore {
/**
 * @brief Internal Representation of a Semaphore
 */
    uint64_t internal;
} sl_os_semaphore;

/**
 * @brief Represents an OS Thread
 */
typedef struct sl_os_thread {
/**
 * @brief Internal Representation of a Thread
 */
    uint64_t internal[2];

} sl_os_thread;

/**
 * @brief Represents an Entry Function for a Thread
 */
typedef void thread_entry(void *data);

/**
 * @brief Represents an Entry Function for a Fiber
 */
typedef void fiber_entry(void *data);

/**
 * @brief Represents an OS Fiber
 */
typedef struct sl_os_fiber {
/**
 * @brief Internal Fiber Index/Representation
 */
    uint64_t internal;

} sl_os_fiber;

/**
 * @brief Abstraction of OS Threading Operations
 */
typedef struct sl_os_thread_api {
/**
 * @brief Creates an OS Semaphore and Sets an Initial Count
 * @param initial_count The Initial Value for the Semaphore
 * @returns The Created Semaphore
 */
    sl_os_semaphore (*init_semaphore)(uint32_t initial_count);

/**
 * @brief Adds a Value to a Specified Semaphore
 * @param sem Semaphore to Add a Value too
 * @param value Integer Value to be Added to Semaphore
 */
    void (*add_semaphore_count)(sl_os_semaphore sem, uint32_t value);

/**
 * @brief Waits For Specified Semaphore to be in a Signalled State
 * @param sem Semaphore to Wait For
 */
    void (*wait_semaphore)(sl_os_semaphore sem);

/**
 * @brief Destroys the Specified Semaphore
 * @param sem Semaphore to be Destroyed
 */
    void (*close_semaphore)(sl_os_semaphore sem);

/**
 * @brief Creates an OS Thread for the given Entry function
 * @param entry Function for the thread to run
 * @param user_data Optional User Defined Data to send to entry function
 * @param stack_size Size of the stack space allocated for this thread
 * @param debug_name Name given to the thread when looking at the debugger
 * @returns The created sl_os_thread handle
 */
    sl_os_thread (*create_os_thread)(thread_entry *entry, void *user_data, uint32_t stack_size,
                               const char *debug_name);

    /**
 * @brief Sets The Current Threads Custom Name
 * @param thread The Thread to Modify
 * @param name The Name To Give to the Thread
 */
    void (*set_thread_name)(const char *name);

    /**
* @brief Gets The Current Threads Custom Name
* @param buffer C String Buffer to be Filled with the Threads Name
* @param size Size of the Buffer/Name
*/
    void (*get_thread_name)(char *buffer, int size);

/**
 * @brief Sets a Threads Affinity. Usually used to lock a thread to a specific core
 * @param thread The thread to be modified
 * @param value The affinity value
 */
    void (*set_thread_affinity)(sl_os_thread thread, uint32_t value);

/**
 * @brief Gets a Thread ID from a given sl_os_thread
 * @param thread The sl_os_thread that we will get the ID of
 * @returns Thread ID Value as uint32_t
 */
    uint32_t (*get_thread_id_from_thread)(sl_os_thread thread);

/**
 * @brief Returns Current Thread ID
 */
    uint32_t (*get_thread_id)(void);

/**
 * @brief Yields Execution to Another Thread
 */
    void (*thread_yield)(void);

/**
 * @brief Sleeps Calling Thread by Seconds
 * @param seconds Amount of time in Seconds to Sleep the Thread
 */
    void (*sleep)(double seconds);

/**
 * @brief Converts the Current Thread to a Fiber
 * @param user_data A pointer to specific user data for this fiber
 * @returns The sl_os_fiber that gets created...
 */
    sl_os_fiber (*thread_to_fiber)(void *user_data);

/**
 * @brief Converts the current running fiber to a thread
 */
    void (*fiber_to_thread)(void);

/**
 * @brief Creates an OS Fiber
 * @param entry A function pointer of type fiber_entry for the Fiber to Call
 * @param user_data A pointer to data that is passed to the entry function
 * @param stack_size The size of the Stack to be allocated for this Fiber
 * @returns The sl_os_fiber that gets created...
 */
    sl_os_fiber (*create_fiber)(fiber_entry *entry, void *user_data, uint32_t stack_size);

/**
 * @brief Destroys an OS Fiber
 * @param fiber The fiber to be destroyed
 */
    void (*destroy_fiber)(sl_os_fiber fiber);

/**
 * @brief Switches From Current Fiber to Specified Fiber
 * @param fiber The fiber to be switched too
 */
    void (*switch_to_fiber)(sl_os_fiber fiber);

/**
 * @brief Returns the current fibers data given at Fiber Creation
 * @returns Current Fiber Data
 */
    void *(*get_fiber_data)(void);

} sl_os_thread_api;

typedef struct sl_os_file {
    uint64_t handle;
    bool valid;

} sl_os_file;

typedef struct sl_os_filesystem_api {

    sl_os_file (*open_file_write)(const char *path);

    sl_os_file (*open_file_read)(const char *path);

    sl_os_file (*open_file_append)(const char *path);

    bool (*file_write)(sl_os_file file, const void *p_buffer, uint64_t size);

    void (*file_close)(sl_os_file file);

} sl_os_filesystem_api;

typedef struct sl_os_info_api
{
    uint32_t (*num_logical_cores)(void);
} sl_os_info_api;

/**
 * @brief Abstraction of Common OS Operations
 */
struct sl_os_api {
    sl_os_thread_api *thread;
    sl_os_filesystem_api *file_system;
    sl_os_info_api* info;
    void (*failed_assert)(const char* file, int line, const char* msg);
};

#define SL_OS_API "sl_os_api"

#ifdef LINKS_SL_BASE
extern struct sl_os_api* sl_os_api;
#endif

#ifdef __cplusplus
}
#endif

#endif //STARLIGHT_OS_H
