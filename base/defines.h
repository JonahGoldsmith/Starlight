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

/*Cross Platform Macro code from https://github.com/septag/rizz */
//Copyright at end of file

#ifndef STARLIGHT_DEFINES_H
#define STARLIGHT_DEFINES_H

#include "platform.h"

//Almost Every File Will Include These Headers
#include <stdbool.h>    // bool
#include <stddef.h>     // NULL, size_t
#include <stdint.h>     // uint32_t, int64_t, etc.
#include <stdarg.h>     // va_list

//Stringize and Concat Macros
#define sl_stringize( x ) #x
#define sl_concat(a, b) a##b

// Statements like:
// #pragma message(Reminder "Fix this problem!")
// Which will cause messages like:
// C:\Source\Project\main.cpp(47): Reminder: Fix this problem!
// to show up during compiles.
#define sl_reminder_make_string( M, L ) M(L)
#define $Line sl_reminder_make_string( sl_stringize, __LINE__ )
#define sl_reminder __FILE__ "(" $Line ") : Reminder: "

//Cross Platform Debug Breaking
#if SL_COMPILER_MSVC
#   define sl_debugbreak() __debugbreak()
#elif SL_COMPILER_CLANG
#    if (__has_builtin(__builtin_debugtrap))
#        define sl_debugbreak() __builtin_debugtrap()
#    else
#        define sl_debugbreak() __builtin_trap()    // this cannot be used in constexpr functions
#    endif
#elif SL_COMPILER_GCC
#    define sl_debugbreak() __builtin_trap()
#endif

//Alignment Macros
#define sl_align_mask(_value, _mask) (((_value) + (_mask)) & ((~0) & (~(_mask))))
#define sl_align_16(_value) sl_align_mask(_value, 0xf)
#define sl_align_256(_value) sl_align_mask(_value, 0xff)
#define sl_align_4096(_value) sl_align_mask(_value, 0xfff)

//Cross Compiler Feature Macros...
//Used for Disabling or Enabling Compiler Features/Warnings
#if defined(__has_feature)
#    define sl_clang_has_feature(_x) __has_feature(_x)
#else
#    define sl_clang_has_feature(_x) 0
#endif    // defined(__has_feature)

#if defined(__has_extension)
#    define sl_clang_has_extension(_x) __has_extension(_x)
#else
#    define sl_clang_has_extension(_x) 0
#endif    // defined(__has_extension)

//Cross Platform Defines
#if SL_COMPILER_GCC || SL_COMPILER_CLANG && !SL_COMPILER_MSVC
#define SL_ATOMIC _Atomic
#    define SL_THREAD_LOCAL __thread
#    define SL_ALLOW_UNUSED __attribute__((unused))
#    define SL_FORCE_INLINE SL_INLINE __attribute__((__always_inline__))
#    define SL_FUNCTION __PRETTY_FUNCTION__
#    define SL_NO_INLINE __attribute__((noinline))
#    define SL_NO_RETURN __attribute__((noreturn))
#    define SL_CONSTFN __attribute__((const))
#    define SL_RESTRICT __restrict__
// https://awesomekling.github.io/Smarter-C++-inlining-with-attribute-flatten/
#    define SL_FLATTEN __attribute__((flatten))     // inline everything in the function body
#    if SL_CRT_MSVC
#        define __stdcall
#    endif    // SL_CRT_MSVC
#    if SL_CONFIG_FORCE_INLINE_DEBUG
#       define SL_INLINE static
#    else
#       define SL_INLINE static inline
#    define SL_NO_VTABLE
#    endif
#elif SL_COMPILER_MSVC
#define SL_ATOMIC
#    define SL_ALLOW_UNUSED
#    define SL_FORCE_INLINE __forceinline
#    define SL_FUNCTION __FUNCTION__
#    define SL_NO_INLINE __declspec(noinline)
#    define SL_NO_RETURN
#    define SL_CONSTFN __declspec(noalias)
#    define SL_RESTRICT __restrict
#    define SL_FLATTEN
#    if SL_CONFIG_FORCE_INLINE_DEBUG
#       define SL_INLINE static
#    else
#       define SL_INLINE static inline
#    endif
#    define SL_NO_VTABLE __declspec(novtable)
#    define SL_THREAD_LOCAL __declspec( thread )
#    ifndef __cplusplus
#       define _Thread_local __declspec(thread)
#    endif // __cplusplus
#else
#    error "Unknown SL_COMPILER_?"
#endif

#if SL_COMPILER_GCC || SL_COMPILER_CLANG
#    define sl_align_decl(_align, _decl) _decl __attribute__((aligned(_align)))
#else
#    define sl_align_decl(_align, _decl) __declspec(align(_align)) _decl
#endif

#if SL_COMPILER_CLANG
#    define SL_PRAGMA_DIAGNOSTIC_PUSH_CLANG_() _Pragma("clang diagnostic push")
#    define SL_PRAGMA_DIAGNOSTIC_POP_CLANG_() _Pragma("clang diagnostic pop")
#    define SL_PRAGMA_DIAGNOSTIC_IGNORED_CLANG(_x) _Pragma(sl_stringize(clang diagnostic ignored _x))
#else
#    define SL_PRAGMA_DIAGNOSTIC_PUSH_CLANG_()
#    define SL_PRAGMA_DIAGNOSTIC_POP_CLANG_()
#    define SL_PRAGMA_DIAGNOSTIC_IGNORED_CLANG(_x)
#endif    // SL_COMPILER_CLANG

#if SL_COMPILER_GCC && SL_COMPILER_GCC >= 40600
#    define SL_PRAGMA_DIAGNOSTIC_PUSH_GCC_() _Pragma("GCC diagnostic push")
#    define SL_PRAGMA_DIAGNOSTIC_POP_GCC_() _Pragma("GCC diagnostic pop")
#    define SL_PRAGMA_DIAGNOSTIC_IGNORED_GCC(_x) _Pragma(sl_stringize(GCC diagnostic ignored _x))
#else
#    define SL_PRAGMA_DIAGNOSTIC_PUSH_GCC_()
#    define SL_PRAGMA_DIAGNOSTIC_POP_GCC_()
#    define SL_PRAGMA_DIAGNOSTIC_IGNORED_GCC(_x)
#endif    // SL_COMPILER_GCC

#if SL_COMPILER_MSVC
#    define SL_PRAGMA_DIAGNOSTIC_PUSH_MSVC_() __pragma(warning(push))
#    define SL_PRAGMA_DIAGNOSTIC_POP_MSVC_() __pragma(warning(pop))
#    define SL_PRAGMA_DIAGNOSTIC_IGNORED_MSVC(_x) __pragma(warning(disable : _x))
#else
#    define SL_PRAGMA_DIAGNOSTIC_PUSH_MSVC_()
#    define SL_PRAGMA_DIAGNOSTIC_POP_MSVC_()
#    define SL_PRAGMA_DIAGNOSTIC_IGNORED_MSVC(_x)
#endif    // SL_COMPILER_MSVC

#if SL_COMPILER_CLANG
#    define SL_PRAGMA_DIAGNOSTIC_PUSH SL_PRAGMA_DIAGNOSTIC_PUSH_CLANG_
#    define SL_PRAGMA_DIAGNOSTIC_POP SL_PRAGMA_DIAGNOSTIC_POP_CLANG_
#    define SL_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC SL_PRAGMA_DIAGNOSTIC_IGNORED_CLANG
#elif SL_COMPILER_GCC
#    define SL_PRAGMA_DIAGNOSTIC_PUSH SL_PRAGMA_DIAGNOSTIC_PUSH_GCC_
#    define SL_PRAGMA_DIAGNOSTIC_POP SL_PRAGMA_DIAGNOSTIC_POP_GCC_
#    define SL_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC SL_PRAGMA_DIAGNOSTIC_IGNORED_GCC
#elif SL_COMPILER_MSVC
#    define SL_PRAGMA_DIAGNOSTIC_PUSH SL_PRAGMA_DIAGNOSTIC_PUSH_MSVC_
#    define SL_PRAGMA_DIAGNOSTIC_POP SL_PRAGMA_DIAGNOSTIC_POP_MSVC_
#    define SL_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC(_x)
#endif    // SL_COMPILER_

//Cross Platform Memset, Memcpy, Memmove, and Memcmp
#ifndef sl_memset
#    include <memory.h>    // memset
#    define sl_memset(_dst, _n, _sz) memset((_dst), (_n), (_sz))
#endif

#ifndef sl_memcpy
#    include <memory.h>    // memcpy
#    define sl_memcpy(_dst, _src, _n) memcpy((_dst), (_src), (_n))
#endif

#ifndef sl_memmove
#    if SL_CRT_MINGW
#       include <string.h>    // memmove
#    else
#       include <memory.h>    // memmove
#    endif // SL_CRT_MINGW
#    define sl_memmove(_dst, _src, _n) memmove((_dst), (_src), (_n))
#endif

#ifndef sl_memcmp
#    include <memory.h>    // memcmp
#    define sl_memcmp(_p1, _p2, _n) memcmp((_p1), (_p2), (_n))
#endif

#define sl_swap(a, b, _type) \
    do {                     \
        _type tmp = a;       \
        a = b;               \
        b = tmp;             \
    } while (0)

#ifndef __cplusplus
#    if SL_COMPILER_CLANG
#        define sl_max(a, b)                  \
            ({                                \
                __typeof__(a) var__a = (a);       \
                __typeof__(b) var__b = (b);       \
                (void)(&var__a == &var__b);   \
                var__a > var__b ? var__a : var__b; \
            })

#        define sl_min(a, b)                  \
            ({                                \
                __typeof__(a) var__a = (a);       \
                __typeof__(b) var__b = (b);       \
                (void)(&var__a == &var__b);   \
                var__a < var__b ? var__a : var__b; \
            })

#        define sl_clamp(v_, min_, max_)                        \
            ({                                                  \
                __typeof__(v_) var__v = (v_);                       \
                __typeof__(min_) var__min = (min_);                 \
                __typeof__(max_) var__max = (max_);                 \
                (void)(&var__min == &var__max);                 \
                var__v = var__v < var__max ? var__v : var__max; \
                var__v > var__min ? var__v : var__min;          \
            })
#elif SL_COMPILER_GCC
#        define sl_max(a, b)                  \
            ({                                \
                typeof(a) var__a = (a);       \
                typeof(b) var__b = (b);       \
                (void)(&var__a == &var__b);   \
                var__a > var__b ? var__a : var__b; \
            })

#        define sl_min(a, b)                  \
            ({                                \
                typeof(a) var__a = (a);       \
                typeof(b) var__b = (b);       \
                (void)(&var__a == &var__b);   \
                var__a < var__b ? var__a : var__b; \
            })

#        define sl_clamp(v_, min_, max_)                        \
            ({                                                  \
                typeof(v_) var__v = (v_);                       \
                typeof(min_) var__min = (min_);                 \
                typeof(max_) var__max = (max_);                 \
                (void)(&var__min == &var__max);                 \
                var__v = var__v < var__max ? var__v : var__max; \
                var__v > var__min ? var__v : var__min;          \
            })
#elif SL_COMPILER_MSVC
// NOTE: Because we have some features lacking in MSVC+C compiler, the max,min,clamp macros does not
// pre-evaluate the expressions So in performance critical code, make sure you pre-evaluate the
// sl_max, sl_min, sl_clamp paramters before passing them to the macros
#        define sl_max(a, b) ((a) > (b) ? (a) : (b))
#        define sl_min(a, b) ((a) < (b) ? (a) : (b))
#        define sl_clamp(v, min_, max_) sl_max(sl_min((v), (max_)), (min_))
#endif    // SL_COMPILER_GCC||SL_COMPILER_CLANG
#else // __cplusplus
template <typename T>
T sl_max(T a, T b);
template <typename T>
T sl_min(T a, T b);
template <typename T>
T sl_clamp(T v, T _min, T _max);

template <>
inline int sl_max(int a, int b)
{
    return (a > b) ? a : b;
}
template <>
inline float sl_max(float a, float b)
{
    return (a > b) ? a : b;
}
template <>
inline double sl_max(double a, double b)
{
    return (a > b) ? a : b;
}
template <>
inline int8_t sl_max(int8_t a, int8_t b)
{
    return (a > b) ? a : b;
}
template <>
inline uint8_t sl_max(uint8_t a, uint8_t b)
{
    return (a > b) ? a : b;
}
template <>
inline int16_t sl_max(int16_t a, int16_t b)
{
    return (a > b) ? a : b;
}
template <>
inline uint16_t sl_max(uint16_t a, uint16_t b)
{
    return (a > b) ? a : b;
}
template <>
inline uint32_t sl_max(uint32_t a, uint32_t b)
{
    return (a > b) ? a : b;
}
template <>
inline int64_t sl_max(int64_t a, int64_t b)
{
    return (a > b) ? a : b;
}
template <>
inline uint64_t sl_max(uint64_t a, uint64_t b)
{
    return (a > b) ? a : b;
}

template <>
inline int sl_min(int a, int b)
{
    return (a < b) ? a : b;
}
template <>
inline float sl_min(float a, float b)
{
    return (a < b) ? a : b;
}
template <>
inline double sl_min(double a, double b)
{
    return (a < b) ? a : b;
}
template <>
inline int8_t sl_min(int8_t a, int8_t b)
{
    return (a < b) ? a : b;
}
template <>
inline uint8_t sl_min(uint8_t a, uint8_t b)
{
    return (a < b) ? a : b;
}
template <>
inline int16_t sl_min(int16_t a, int16_t b)
{
    return (a < b) ? a : b;
}
template <>
inline uint16_t sl_min(uint16_t a, uint16_t b)
{
    return (a < b) ? a : b;
}
template <>
inline uint32_t sl_min(uint32_t a, uint32_t b)
{
    return (a < b) ? a : b;
}
template <>
inline int64_t sl_min(int64_t a, int64_t b)
{
    return (a < b) ? a : b;
}
template <>
inline uint64_t sl_min(uint64_t a, uint64_t b)
{
    return (a < b) ? a : b;
}

template <>
inline int sl_clamp(int v, int _min, int _max)
{
    return sl_max(sl_min(v, _max), _min);
}
template <>
inline float sl_clamp(float v, float _min, float _max)
{
    return sl_max(sl_min(v, _max), _min);
}
template <>
inline double sl_clamp(double v, double _min, double _max)
{
    return sl_max(sl_min(v, _max), _min);
}
template <>
inline int8_t sl_clamp(int8_t v, int8_t _min, int8_t _max)
{
    return sl_max(sl_min(v, _max), _min);
}
template <>
inline uint8_t sl_clamp(uint8_t v, uint8_t _min, uint8_t _max)
{
    return sl_max(sl_min(v, _max), _min);
}
template <>
inline int16_t sl_clamp(int16_t v, int16_t _min, int16_t _max)
{
    return sl_max(sl_min(v, _max), _min);
}
template <>
inline uint16_t sl_clamp(uint16_t v, uint16_t _min, uint16_t _max)
{
    return sl_max(sl_min(v, _max), _min);
}
template <>
inline uint32_t sl_clamp(uint32_t v, uint32_t _min, uint32_t _max)
{
    return sl_max(sl_min(v, _max), _min);
}
template <>
inline int64_t sl_clamp(int64_t v, int64_t _min, int64_t _max)
{
    return sl_max(sl_min(v, _max), _min);
}
template <>
inline uint64_t sl_clamp(uint64_t v, uint64_t _min, uint64_t _max)
{
    return sl_max(sl_min(v, _max), _min);
}
#endif


#define sl_var(_name) sl_concat(_name, __LINE__)
#define sl_defer(_start, _end) for (int sl_var(_i_) = (_start, 0); !sl_var(_i_); (sl_var(_i_) += 1), _end)
#define sl_scope(_end) for (int _sx_var(_i_) = 0; !_sx_var(_i_); (_sx_var(_i_) += 1), _end)

#define SL_KILOBYTES(x) (x * 1024)
#define SL_MEGABYTES(x) (SL_KILOBYTES(x) * 1024)
#define SL_GIGABYTES(x) (SL_MEGABYTES(x) * 1024)

#define SL_TO_KILOBYTES(x) (x/1024)

#endif //STARLIGHT_DEFINES_H

//
// Copyright 2018 Sepehr Taghdisian (septag@github). All rights reserved.
// License: https://github.com/septag/sx#license-bsd-2-clause
//