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

#ifndef SL_ARRAY_INL
#define SL_ARRAY_INL

#include "defines.h"
#include "memory/allocator.h"

/**
 * @brief Get The Header Data of a Array
 * @param t The Array
 * @returns The Header Data of the specified array
 */
#define sl_array_get_header(t)  ((sl_array_header*) (t) - 1)

/**
 * @brief Set The Capacity of a Array
 * @param alloc Allocator to be used
 * @param a The array
 * @param n New Capacity of the array
 */
#define sl_array_set_capacity(alloc, a,n)   (sl_array_grow(alloc, a,0,n))
/**
 * @brief Resize a Array
* @param alloc Allocator to be used
* @param a The array
* @param n The new size of the array
 */
#define sl_array_resize(alloc, a,n)   ((sl_array_capacity(a) < (size_t) (n) ? sl_array_set_capacity(alloc, (a),(size_t)(n)),0 : 0), (a) ? sl_array_get_header(a)->length = (size_t) (n) : 0)

/**
 * @brief Get The Capacity of a Array
 * @param a The array
 * @returns The capacity as a size_t
 */
#define sl_array_capacity(a)        ((a) ? sl_array_get_header(a)->capacity : 0)

/**
 * @brief Get The Size of a Array
 * @param a The array
 * @returns The Size(amount of objects inside) as a size_t
 */
#define sl_array_size(a)       ((a) ?             sl_array_get_header(a)->length : 0)
/**
 * @brief Get The Amount of Bytes inside of a Array.
 * This is used for copying memory from an array
 * @param a The array
 * @returns The amount of bytes needed for memcpy as size_t
 */
#define sl_array_bytes(a) (sl_array_size(a) * sizeof(*(a)))
/**
 * @brief Push an Object into a Array
* @param alloc Allocator to be used
* @param a The array
* @param v The object to push
 */
#define sl_array_push(alloc, a,v)      (sl_array_mayb_grow(alloc, a,1), (a)[sl_array_get_header(a)->length++] = (v))

/**
 * @brief Pop the last object from a Array
* @param a The array
 */
#define sl_array_pop(a)        (sl_array_get_header(a)->length--, (a)[sl_array_get_header(a)->length])
/**
 * @brief Get the last object inside of a Array
* @param a The array
 */
#define sl_array_last(a)       ((a)[sl_array_get_header(a)->length-1])
/**
 * @brief Get a pointer at the end of a Array
* @param a The array
 */
#define sl_array_end(a) 		((a) ? (a)+sl_array_size(a) : 0)
/**
 * @brief Free all memory associated with a Array
 * If objects inside buffer are heap allocated objects,
 * the user must free them on their own
 * @param alloc The allocator to be used
* @param a The array
 */
#define sl_array_free(alloc, a)       ((void) ((a) ? sl_free(alloc, sl_array_get_header(a)) : (void*)0), (a)=NULL)


#define sl_array_del(a,i)      sl_array_deln(a,i,1)
#define sl_array_addn(a,n)     ((void)(sl_array_addnindex(a, n)))    // deprecated, use one of the following instead:
#define sl_array_addnptr(alloc, a,n)  (sl_array_mayb_grow(alloc, a,n), (n) ? (sl_array_get_header(a)->length += (n), &(a)[sl_array_get_header(a)->length-(n)]) : (a))
#define sl_array_addnindex(alloc, a,n)(sl_array_mayb_grow(alloc, a,n), (n) ? (sl_array_get_header(a)->length += (n), sl_array_get_header(a)->length-(n)) : sl_array_size(a))
#define sl_array_addnoff       sl_array_addnindex
#define sl_array_deln(a,i,n)   (memmove(&(a)[i], &(a)[(i)+(n)], sizeof *(a) * (sl_array_get_header(a)->length-(n)-(i))), sl_array_get_header(a)->length -= (n))
#define sl_array_delswap(a,i)  ((a)[i] = sl_array_last(a), sl_array_get_header(a)->length -= 1)
#define sl_array_insn(a,i,n)   (sl_array_addn((a),(n)), memmove(&(a)[(i)+(n)], &(a)[i], sizeof *(a) * (sl_array_get_header(a)->length-(n)-(i))))
#define sl_array_isn(a,i,v)    (sl_array_insn((a),(i),1), (a)[i]=(v))

#define sl_array_mayb_grow(alloc, a,n)  ((!(a) || sl_array_get_header(a)->length + (n) > sl_array_get_header(a)->capacity) \
                                  ? (sl_array_grow(alloc, a,n,0),0) : 0)

#define sl_array_grow(alloc, a,b,c)   ((a) = sl_array_grow_wrapper(alloc, (a), sizeof *(a), (b), (c), SL_FUNCTION, __FILE__, __LINE__))

/**
 * @brief Representation of Array Header Data
 */
typedef struct
{
	/**
 * @brief Amount of objects inside of Array
 */
			  size_t      length;
			  /**
 * @brief Max amount of objects inside Array
 */
			  size_t      capacity;
} sl_array_header;

		  SL_FORCE_INLINE void *sl_array_grow_internal(sl_allocator* alloc, void *a, size_t elemsize, size_t addlen, size_t min_cap, const char* func, const char* file, uint32_t line)
		  {
			  sl_array_header temp={0}; // force debugging
			  void *b;
			  size_t min_len = sl_array_size(a) + addlen;
			  (void) sizeof(temp);

			  // compute the minimum capacity needed
			  if (min_len > min_cap)
				  min_cap = min_len;

			  if (min_cap <= sl_array_capacity(a))
				  return a;

			  // increase needed capacity to guarantee O(1) amortized
			  if (min_cap < 2 * sl_array_capacity(a))
				  min_cap = 2 * sl_array_capacity(a);
			  else if (min_cap < 4)
				  min_cap = 4;

			  b = alloc->realloc(alloc, (a) ? sl_array_get_header(a) : 0, elemsize * min_cap + sizeof(sl_array_header), 0, func, file, line);

			  b = (char *) b + sizeof(sl_array_header);
			  if (a == NULL) {
				  sl_array_get_header(b)->length = 0;
			  }

			  sl_array_get_header(b)->capacity = min_cap;

			  return b;
		  }


#ifdef __cplusplus
		  // in C we use implicit assignment from these void*-returning functions to T*.
		  // in C++ these templates make the same code work
		  template<class T> static T * sl_array_grow_wrapper(sl_allocator* alloc, T *a, size_t elemsize, size_t addlen, size_t min_cap, const char* func, const char* file, uint32_t line) {
			  return (T*)sl_array_grow_internal(alloc, (void *)a, elemsize, addlen, min_cap, func, file, line);
		  }
#else
#define sl_array_grow_wrapper            sl_array_grow_internal
#endif

#define SL_ARRAY(type, name) type* name

		  /*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2019 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/

#endif//SL_ARRAY_INL
