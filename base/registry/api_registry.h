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

#ifndef API_REGISTRY_H
#define API_REGISTRY_H

#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interface of the Api Registry
 * Set and Get Interfaces across Modules and Main Project
 */
struct sl_api_registry
{

	/**
 * @brief Set The Interface Pointers at A Given Name
 * @param name Name of the Interface
 * @param p_interf Pointer to Struct filled with function pointers
 * @param size size of the struct being passed in
 */
	void (*set)(const char* name, void* p_interf, uint32_t size);
	/**
 * @brief Remove Api with given name
 * @param name Name of the Interface
 */
	void (*remove)(const char* name);
	/**
* @brief Get Api with given name
* @param name Name of the Interface
*/
	void* (*get)(const char* name);
};

#define SL_API_REGISTRY_API "sl_global_api_registry"

#define SL_REGISTRY_SET_API(name, api) sl_global_api_registry->set(name, api, sizeof(*api))

#ifdef LINKS_SL_BASE

extern struct sl_api_registry* sl_global_api_registry;

#endif

#ifdef __cplusplus
}
#endif

#endif//API_REGISTRY_H
