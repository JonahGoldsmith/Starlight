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

#include "sprintf.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "third-party/stb/stb_sprintf.h"

static int internal_sprintf(char* buf, const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    int res = stbsp_vsprintf(buf, fmt, va);
    va_end(va);
    return res;
}

static int internal_snprintf(char* buf, int count, const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    int res = stbsp_vsnprintf(buf, count, fmt, va);
    va_end(va);
    return res;
}

static int internal_vsprintf(char* buf, const char* fmt, va_list va)
{
    return stbsp_vsprintf(buf, fmt, va);
}

static int internal_vsnprintf(char* buf, int count, const char* fmt, va_list va)
{
    return stbsp_vsnprintf(buf, count, fmt, va);
}

struct sl_sprintf_api sprintf_api = {
        internal_sprintf,
        internal_snprintf,
        internal_vsprintf,
        internal_vsnprintf
};

struct sl_sprintf_api* sl_sprintf_api = &sprintf_api;