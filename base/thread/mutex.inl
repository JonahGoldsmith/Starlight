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

#ifndef STARLIGHT_THREADING_INL
#define STARLIGHT_THREADING_INL

#include "defines.h"

#ifndef STARLIGHT_OS_H
typedef struct sl_os_mutex
{
#if SL_PLATFORM_WINDOWS
    uint64_t handle;
#else
    uint64_t handle[8];
#endif
}sl_os_mutex;
#endif

#if SL_PLATFORM_OSL
#include <pthread.h>

SL_FORCE_INLINE void sl_create_mutex(sl_os_mutex* m)
{
    int res = pthread_mutex_init((pthread_mutex_t *)m, NULL);
}

SL_FORCE_INLINE void sl_lock_mutex(sl_os_mutex* m)
{
    int res = pthread_mutex_lock((pthread_mutex_t *)m);
}

SL_FORCE_INLINE void sl_unlock_mutex(sl_os_mutex *m)
{
    int res = pthread_mutex_unlock((pthread_mutex_t *)m);
}

SL_FORCE_INLINE void sl_destroy_mutex(sl_os_mutex *m)
{
    int res = pthread_mutex_destroy((pthread_mutex_t *)m);
    *m = (sl_os_mutex){ 0 };
}

#endif

#define SL_MUTEX_LOCK(_lock) sl_defer(sl_lock_mutex(&_lock), sl_unlock_mutex(&_lock))

#endif //STARLIGHT_THREADING_INL
