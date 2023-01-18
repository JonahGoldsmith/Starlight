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

#ifndef STARLIGHT_PLATFORM_H
#define STARLIGHT_PLATFORM_H

/*Platform Detection code from https://github.com/septag/rizz*/
//Copyright at end of file
// Another useful reference: https://sourceforge.net/p/predef/wiki/Home/



// Architecture
#define SL_ARCH_32BIT 0
#define SL_ARCH_64BIT 0

// Compiler
#define SL_COMPILER_CLANG 0
#define SL_COMPILER_CLANG_ANALYZER 0
#define SL_COMPILER_CLANG_CL 0
#define SL_COMPILER_GCC 0
#define SL_COMPILER_MSVC 0

// Endianess
#define SL_CPU_ENDIAN_BIG 0
#define SL_CPU_ENDIAN_LITTLE 0

// CPU
#define SL_CPU_ARM 0
#define SL_CPU_JIT 0
#define SL_CPU_MIPS 0
#define SL_CPU_PPC 0
#define SL_CPU_RISCV 0
#define SL_CPU_X86 0

// C Runtime
#define SL_CRT_BIONIC 0
#define SL_CRT_GLIBC 0
#define SL_CRT_LIBCXX 0
#define SL_CRT_MINGW 0
#define SL_CRT_MSVC 0
#define SL_CRT_NEWLIB 0

#ifndef SL_CRT_MUSL
#    define SL_CRT_MUSL 0
#endif    // SL_CRT_MUSL

#ifndef SL_CRT_NONE
#    define SL_CRT_NONE 0
#endif    // SL_CRT_NONE

// Platform
#define SL_PLATFORM_ANDROID 0
#define SL_PLATFORM_BSD 0
#define SL_PLATFORM_EMSCRIPTEN 0
#define SL_PLATFORM_HURD 0
#define SL_PLATFORM_IOS 0
#define SL_PLATFORM_LINUX 0
#define SL_PLATFORM_NX 0
#define SL_PLATFORM_OSL 0
#define SL_PLATFORM_PS4 0
#define SL_PLATFORM_RPI 0
#define SL_PLATFORM_WINDOWS 0
#define SL_PLATFORM_WINRT 0
#define SL_PLATFORM_XBOXONE 0

// C11 thread_local, Because we are missing threads.h
#if __STDC_VERSION__ >= 201112L
#    define thread_local _Thread_local
#endif

// http://sourceforge.net/apps/mediawiki/predef/index.php?title=Compilers
#if defined(__clang__)
// clang defines __GNUC__ or _MSC_VER
#    undef SL_COMPILER_CLANG
#    define SL_COMPILER_CLANG \
        (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#    if defined(__clang_analyzer__)
#        undef SL_COMPILER_CLANG_ANALYZER
#        define SL_COMPILER_CLANG_ANALYZER 1
#    endif    // defined(__clang_analyzer__)
#    if defined(_MSC_VER)
#        undef SL_COMPILER_MSVC
#        define SL_COMPILER_MSVC _MSC_VER
#        undef SL_COMPILER_CLANG_CL
#        define SL_COMPILER_CLANG_CL SL_COMPILER_CLANG
#    endif
#elif defined(_MSC_VER)
#    undef SL_COMPILER_MSVC
#    define SL_COMPILER_MSVC _MSC_VER
#elif defined(__GNUC__)
#    undef SL_COMPILER_GCC
#    define SL_COMPILER_GCC (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
#    error "SL_COMPILER_* is not defined!"
#endif    //

// http://sourceforge.net/apps/mediawiki/predef/index.php?title=Architectures
#if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM)
#    undef SL_CPU_ARM
#    define SL_CPU_ARM 1
#    define SL_CACHE_LINE_SIZE 64
#elif defined(__MIPSEL__) || defined(__mips_isa_rev) || defined(__mips64)
#    undef SL_CPU_MIPS
#    define SL_CPU_MIPS 1
#    define SL_CACHE_LINE_SIZE 64
#elif defined(_M_PPC) || defined(__powerpc__) || defined(__powerpc64__)
#    undef SL_CPU_PPC
#    define SL_CPU_PPC 1
#    define SL_CACHE_LINE_SIZE 128
#elif defined(__riscv) || defined(__riscv__) || defined(RISCVEL)
#    undef SL_CPU_RISCV
#    define SL_CPU_RISCV 1
#    define SL_CACHE_LINE_SIZE 64
#elif defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#    undef SL_CPU_X86
#    define SL_CPU_X86 1
#    define SL_CACHE_LINE_SIZE 64
#else    // PNaCl doesn't have CPU defined.
#    undef SL_CPU_JIT
#    define SL_CPU_JIT 1
#    define SL_CACHE_LINE_SIZE 64
#endif    //

#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__) || defined(__64BIT__) || \
    defined(__mips64) || defined(__powerpc64__) || defined(__ppc64__) || defined(__LP64__)
#    undef SL_ARCH_64BIT
#    define SL_ARCH_64BIT 64
#else
#    undef SL_ARCH_32BIT
#    define SL_ARCH_32BIT 32
#endif    //

#if SL_CPU_PPC
#    undef SL_CPU_ENDIAN_BIG
#    define SL_CPU_ENDIAN_BIG 1
#else
#    undef SL_CPU_ENDIAN_LITTLE
#    define SL_CPU_ENDIAN_LITTLE 1
#endif    // SL_PLATFORM_

// http://sourceforge.net/apps/mediawiki/predef/index.php?title=Operating_Systems
#if defined(_DURANGO) || defined(_XBOX_ONE)
#    undef SL_PLATFORM_XBOXONE
#    define SL_PLATFORM_XBOXONE 1
#elif defined(__ANDROID__) || defined(ANDROID)
// Android compiler defines __linux__
#    include <sys/cdefs.h>    // Defines __BIONIC__ and includes android/api-level.h
#    undef SL_PLATFORM_ANDROID
#    define SL_PLATFORM_ANDROID __ANDROID_API__
#elif defined(_WIN32) || defined(_WIN64)
// http://msdn.microsoft.com/en-us/library/6sehtctf.aspx
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif    // NOMINMAX
//  If _USING_V110_SDK71_ is defined it means we are using the v110_xp or v120_xp toolset.
#    if defined(_MSC_VER) && (_MSC_VER >= 1700) && (!_USING_V110_SDK71_)
#        include <winapifamily.h>
#    endif    // defined(_MSC_VER) && (_MSC_VER >= 1700) && (!_USING_V110_SDK71_)
#    if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#        undef SL_PLATFORM_WINDOWS
#        if !defined(WINVER) && !defined(_WIN32_WINNT)
#            if SL_ARCH_64BIT
//				When building 64-bit target Win7 and above.
#                define WINVER 0x0601
#                define _WIN32_WINNT 0x0601
#            else
//				Windows Server 2003 with SP1, Windows XP with SP2 and above
#                define WINVER 0x0502
#                define _WIN32_WINNT 0x0502
#            endif    // SL_ARCH_64BIT
#        endif        // !defined(WINVER) && !defined(_WIN32_WINNT)
#        define SL_PLATFORM_WINDOWS _WIN32_WINNT
#    else
#        undef SL_PLATFORM_WINRT
#        define SL_PLATFORM_WINRT 1
#    endif
#elif defined(__VCCOREVER__) || defined(__RPI__)
// RaspberryPi compiler defines __linux__
#    undef SL_PLATFORM_RPI
#    define SL_PLATFORM_RPI 1
#elif defined(__linux__)
#    undef SL_PLATFORM_LINUX
#    define SL_PLATFORM_LINUX 1
#elif defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) || \
    defined(__ENVIRONMENT_TV_OS_VERSION_MIN_REQUIRED__)
#    undef SL_PLATFORM_IOS
#    define SL_PLATFORM_IOS 1
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
#    undef SL_PLATFORM_OSL
#    define SL_PLATFORM_OSL __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
#elif defined(__EMSCRIPTEN__)
#    undef SL_PLATFORM_EMSCRIPTEN
#    define SL_PLATFORM_EMSCRIPTEN 1
#elif defined(__ORBIS__)
#    undef SL_PLATFORM_PS4
#    define SL_PLATFORM_PS4 1
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
#    undef SL_PLATFORM_BSD
#    define SL_PLATFORM_BSD 1
#elif defined(__GNU__)
#    undef SL_PLATFORM_HURD
#    define SL_PLATFORM_HURD 1
#elif defined(__NX__)
#    undef SL_PLATFORM_NX
#    define SL_PLATFORM_NX 1
#endif    //

#if !SL_CRT_NONE
// https://sourceforge.net/p/predef/wiki/Libraries/
#    if defined(__BIONIC__)
#        undef SL_CRT_BIONIC
#        define SL_CRT_BIONIC 1
#    elif defined(_MSC_VER)
#        undef SL_CRT_MSVC
#        define SL_CRT_MSVC 1
#    elif defined(__GLIBC__)
#        undef SL_CRT_GLIBC
#        define SL_CRT_GLIBC (__GLIBC__ * 10000 + __GLIBC_MINOR__ * 100)
#    elif defined(__MINGW32__) || defined(__MINGW64__)
#        undef SL_CRT_MINGW
#        define SL_CRT_MINGW 1
#    elif defined(__apple_build_version__) || defined(__ORBIS__) || defined(__EMSCRIPTEN__) || \
        defined(__llvm__)
#        undef SL_CRT_LIBCXX
#        define SL_CRT_LIBCXX 1
#    endif    //

#    if !SL_CRT_BIONIC && !SL_CRT_GLIBC && !SL_CRT_LIBCXX && !SL_CRT_MINGW && !SL_CRT_MSVC && \
        !SL_CRT_MUSL && !SL_CRT_NEWLIB
#        undef SL_CRT_NONE
#        define SL_CRT_NONE 1
#    endif    // SL_CRT_*
#endif        // !SL_CRT_NONE

#define SL_PLATFORM_POSIX                                                                         \
    (0 || SL_PLATFORM_ANDROID || SL_PLATFORM_BSD || SL_PLATFORM_EMSCRIPTEN || SL_PLATFORM_HURD || \
     SL_PLATFORM_IOS || SL_PLATFORM_LINUX || SL_PLATFORM_NX || SL_PLATFORM_OSL ||                 \
     SL_PLATFORM_PS4 || SL_PLATFORM_RPI)

#define SL_PLATFORM_NONE                                                                           \
    !(0 || SL_PLATFORM_ANDROID || SL_PLATFORM_BSD || SL_PLATFORM_EMSCRIPTEN || SL_PLATFORM_HURD || \
      SL_PLATFORM_IOS || SL_PLATFORM_LINUX || SL_PLATFORM_NX || SL_PLATFORM_OSL ||                 \
      SL_PLATFORM_PS4 || SL_PLATFORM_RPI || SL_PLATFORM_WINDOWS ||        \
      SL_PLATFORM_WINRT || SL_PLATFORM_XBOXONE)

#if SL_COMPILER_GCC
#    define SL_COMPILER_NAME                                                             \
        "GCC " SL_stringize(__GNUC__) "." SL_stringize(__GNUC_MINOR__) "." SL_stringize( \
            __GNUC_PATCHLEVEL__)
#elif SL_COMPILER_CLANG
#    define SL_COMPILER_NAME                                                                       \
        "Clang " SL_stringize(__clang_major__) "." SL_stringize(__clang_minor__) "." SL_stringize( \
            __clang_patchlevel__)
#elif SL_COMPILER_MSVC
#    if SL_COMPILER_MSVC >= 1920    // Visual Studio 2019
#        define SL_COMPILER_NAME "MSVC 16.0"
#    elif SL_COMPILER_MSVC >= 1910    // Visual Studio 2017
#        define SL_COMPILER_NAME "MSVC 15.0"
#    elif SL_COMPILER_MSVC >= 1900    // Visual Studio 2015
#        define SL_COMPILER_NAME "MSVC 14.0"
#    elif SL_COMPILER_MSVC >= 1800    // Visual Studio 2013
#        define SL_COMPILER_NAME "MSVC 12.0"
#    elif SL_COMPILER_MSVC >= 1700    // Visual Studio 2012
#        define SL_COMPILER_NAME "MSVC 11.0"
#    elif SL_COMPILER_MSVC >= 1600    // Visual Studio 2010
#        define SL_COMPILER_NAME "MSVC 10.0"
#    elif SL_COMPILER_MSVC >= 1500    // Visual Studio 2008
#        define SL_COMPILER_NAME "MSVC 9.0"
#    else
#        define SL_COMPILER_NAME "MSVC"
#    endif    //
#endif        // SL_COMPILER_

#if SL_PLATFORM_ANDROID
#    define SL_PLATFORM_NAME "Android " SL_stringize(SL_PLATFORM_ANDROID)
#elif SL_PLATFORM_BSD
#    define SL_PLATFORM_NAME "BSD"
#elif SL_PLATFORM_EMSCRIPTEN
#    define SL_PLATFORM_NAME                                           \
        "asm.js " SL_stringize(__EMSCRIPTEN_major__) "." SL_stringize( \
            __EMSCRIPTEN_minor__) "." SL_stringize(__EMSCRIPTEN_tiny__)
#elif SL_PLATFORM_HURD
#    define SL_PLATFORM_NAME "Hurd"
#elif SL_PLATFORM_IOS
#    define SL_PLATFORM_NAME "iOS"
#elif SL_PLATFORM_LINUX
#    define SL_PLATFORM_NAME "Linux"
#elif SL_PLATFORM_NONE
#    define SL_PLATFORM_NAME "None"
#elif SL_PLATFORM_NX
#    define SL_PLATFORM_NAME "NX"
#elif SL_PLATFORM_OSL
#    define SL_PLATFORM_NAME "OSL"
#elif SL_PLATFORM_PS4
#    define SL_PLATFORM_NAME "PlayStation 4"
#elif SL_PLATFORM_RPI
#    define SL_PLATFORM_NAME "RaspberryPi"
#elif SL_PLATFORM_WINDOWS
#    define SL_PLATFORM_NAME "Windows"
#elif SL_PLATFORM_WINRT
#    define SL_PLATFORM_NAME "WinRT"
#elif SL_PLATFORM_XBOXONE
#    define SL_PLATFORM_NAME "Xbox One"
#else
#    error "Unknown SL_PLATFORM!"
#endif    // SL_PLATFORM_

#define SL_PLATFORM_APPLE (0 || SL_PLATFORM_IOS || SL_PLATFORM_OSL)

#if SL_CPU_ARM
#    define SL_CPU_NAME "ARM"
#elif SL_CPU_JIT
#    define SL_CPU_NAME "JIT-VM"
#elif SL_CPU_MIPS
#    define SL_CPU_NAME "MIPS"
#elif SL_CPU_PPC
#    define SL_CPU_NAME "PowerPC"
#elif SL_CPU_RISCV
#    define SL_CPU_NAME "RISC-V"
#elif SL_CPU_X86
#    define SL_CPU_NAME "x86"
#endif    // SL_CPU_

#if SL_CRT_BIONIC
#    define SL_CRT_NAME "Bionic libc"
#elif SL_CRT_GLIBC
#    define SL_CRT_NAME "GNU C Library"
#elif SL_CRT_MSVC
#    define SL_CRT_NAME "MSVC C Runtime"
#elif SL_CRT_MINGW
#    define SL_CRT_NAME "MinGW C Runtime"
#elif SL_CRT_LIBCXX
#    define SL_CRT_NAME "Clang C Library"
#elif SL_CRT_NEWLIB
#    define SL_CRT_NAME "Newlib"
#elif SL_CRT_MUSL
#    define SL_CRT_NAME "musl libc"
#elif SL_CRT_NONE
#    define SL_CRT_NAME "None"
#else
#    error "Unknown SL_CRT!"
#endif    // SL_CRT_

#if SL_ARCH_32BIT
#    define SL_ARCH_NAME "32-bit"
#elif SL_ARCH_64BIT
#    define SL_ARCH_NAME "64-bit"
#endif    // SL_ARCH_


#define SL_PLATFORM_MOBILE (SL_PLATFORM_ANDROID || SL_PLATFORM_IOS)
#define SL_PLATFORM_PC (SL_PLATFORM_WINDOWS || SL_PLATFORM_LINUX || SL_PLATFORM_OSL)

#endif //STARLIGHT_PLATFORM_H


// Copyright 2018 Sepehr Taghdisian (septag@github). All rights reserved.
// License: https://github.com/septag/SL#license-bsd-2-clause
//
// parts of this code is copied from bx library: https://github.com/bkaradzic/bx
// Copyright 2011-2019 Branimir Karadzic. All rights reserved.
// License: https://github.com/bkaradzic/bx#license-bsd-2-clause