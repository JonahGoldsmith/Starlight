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

#ifndef STARLIGHT_SPRINTF_H
#define STARLIGHT_SPRINTF_H

#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sl_sprintf_api
{
    int (*print_unsafe)(char* buf, const char* fmt, ...);
    int (*print)(char* buf, int count, const char* fmt, ...);
    int (*vprint_unsafe)(char* buf, const char* fmt, va_list va);
    int (*vprint)(char* buf, int count, const char* fmt, va_list va);
};

#define SL_SPRINTF_API "sl_sprintf_api"

#ifdef LINKS_SL_BASE
    extern struct sl_sprintf_api* sl_sprintf_api;
#endif

#ifdef __cplusplus
    }
#endif

#define sl_sprintf(buf, fmt, ...) sl_sprintf_api->print_unsafe(buf, fmt, __VA_ARGS__)

#define sl_snprintf(buf, count, fmt, ...) sl_sprintf_api->print(buf, count, fmt, __VA_ARGS__)

#define sl_vsprintf(buf, fmt, va) sl_sprintf_api->vprint_unsafe(buf, fmt, va)

#define sl_vsnprintf(buf, count, fmt, va) sl_sprintf_api->vprint(buf, count, fmt, va)

#endif //STARLIGHT_SPRINTF_H
