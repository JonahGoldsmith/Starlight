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

#ifndef STARLIGHT_SPINLOCK_INL
#define STARLIGHT_SPINLOCK_INL

#include "atomics.inl"

#if SL_PLATFORM_OSL
#include <sched.h>
#endif

/**
 * @brief Opaque Representation of an Atomic SpinLock
 */
typedef struct sl_spinlock
{
    atomic_bool locked;
}sl_spinlock;


/**
 * @brief Initialize an Atomic SpinLock in an Unlocked State! This is NEEDED!
 * @param p_lock A Pointer to the Lock To be Initialize
 */
SL_FORCE_INLINE void sl_spinlock_init(sl_spinlock * p_lock)
{
    p_lock->locked = 0;
}

/**
 * @brief Lock a Specific SpinLock. If Already Locked Yields the Current Thread Until Unlocked.
 * @param p_lock A Pointer to the Lock
 */
SL_FORCE_INLINE void sl_spinlock_lock(sl_spinlock * p_lock)
{

    for(;;){
        if(!atomic_exchange_explicit(&p_lock->locked, 1, memory_order_acquire))
        {
            return;
        }

        while(atomic_load_explicit(&p_lock->locked, memory_order_relaxed))
        {
#if SL_PLATFORM_WINDOWS
            YieldProcessor();
#elif SL_PLATFORM_OSL
            sched_yield();
#endif
        }

    }

}

/**
 * @brief Unlocks a Specific SpinLock
 * @param p_lock A Pointer to the Lock
 */
SL_FORCE_INLINE void sl_spinlock_unlock(sl_spinlock* p_lock)
{
    atomic_store_explicit(&p_lock->locked, 0, memory_order_release);
}

#endif //STARLIGHT_SPINLOCK_INL
