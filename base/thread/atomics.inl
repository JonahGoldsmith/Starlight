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

#ifndef STARLIGHT_ATOMICS_INL
#define STARLIGHT_ATOMICS_INL

#include "defines.h"

#ifdef __cplusplus
#include "atomic"
typedef std::atomic<uint64_t> sl_atomic_uint64_t;
typedef std::atomic<uint32_t> sl_atomic_uint32_t;
typedef std::atomic<bool> sl_atomic_bool;

using std::memory_order_relaxed;
using std::memory_order_release;
using std::memory_order_acquire;
using std::atomic_store_explicit;
using std::atomic_load_explicit;
using std::atomic_compare_exchange_weak_explicit;
using std::atomic_exchange_explicit;
using std::atomic_fetch_sub;
using std::atomic_fetch_add;
using std::atomic_store;
#else
#pragma message(sl_reminder "Come Back On Windows!")
#if SL_COMPILER_MSVC && !SL_COMPILER_CLANG

#include "third-party/c89atomic/c89atomic.h"

#elif SL_COMPILER_CLANG || SL_COMPILER_GCC

#include "stdatomic.h"

typedef atomic_bool sl_atomic_bool;

typedef _Atomic(uint32_t) sl_atomic_uint32_t;

typedef _Atomic(uint64_t) sl_atomic_uint64_t;

#endif

#endif

#endif //STARLIGHT_ATOMICS_INL
