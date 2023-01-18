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


#ifndef STARLIGHT_ASSERTIONS_INL
#define STARLIGHT_ASSERTIONS_INL

#include "defines.h"
#include <stdio.h>

#define SL_ENABLE_ASSERTS 1

#if SL_PLATFORM_OSL
SL_FORCE_INLINE void sl_failed_assert(const char* file, int line, const char* statement)
{
	static bool debug = true;

	if (debug)
	{
		printf("Failed: (%s)\n\nFile: %s\nLine: %d\n\n", statement, file, line);
		sl_debugbreak();
	}
}
#endif


#if SL_ENABLE_ASSERTS

#define SL_ASSERT(b, message)									\
	do												\
	{												\
		if (!(b))									\
		{											\
			sl_failed_assert(__FILE__, __LINE__, message);	\
		}											\
	} while(0)

#else
//-V:ASSERT:568
#define ASSERT(b)									\
	do { (void)sizeof(b); } while(0)
#endif

#endif//STARLIGHT_ASSERTIONS_INL
