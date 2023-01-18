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

#include "api_registry.h"

#include "base/data_structures/hash.inl"

struct internal_api {
	void* functions[128];
};

static size_t g_hashed_names[128];
static const char* g_api_names[128]; //Unused for now... maybe not useful
static void*g_api_pointers[128]; //Unused for now... maybe not useful
static struct internal_api g_apis[128];
static uint32_t g_num_apis = 1;

SL_INLINE uint32_t search_for_api_name_and_hash(const char* name, size_t hash)
{
	//Check To See if Hashed Name Exists in the Array of Hashed Names
	for(uint32_t i = 0; i < g_num_apis; i++)
	{
		if(g_hashed_names[i]==hash)
		{
			//If it exists... return the index of the array.
			return i;
		}
	}
	//If it doesnt exist...

	//Add the name to the name array
	g_api_names[g_num_apis] = name;
	//Add the hashed name to the hashed name array
	g_hashed_names[g_num_apis] = hash;
	//Increment number of apis in the system
	g_num_apis += 1;
	//Return the previous number of apis (the index we just added too)
	return g_num_apis - 1;
}

void set_api(const char* name, void* pInterf, uint32_t size)
{
	//Get the hash of the string
	const size_t hash = sl_hash_string(name, 0);

	//Search for the api... will add a new api to the array if not found
	const uint32_t index = search_for_api_name_and_hash(name, hash);

	//Set the function pointers
	g_api_pointers[index] = (void *)pInterf;
	memset(g_apis[index].functions, 0, sizeof(g_apis[index].functions));
	memcpy(g_apis[index].functions, pInterf, size);
}

void remove_api(const char* name)
{
	//TODO: Implement Removing Api's
}

void* get_api(const char* name)
{
	//Get the hash of the string
	const size_t hash = sl_hash_string(name, 0);

	//Search for the api... will add a new api to the array if not found
	const uint32_t index = search_for_api_name_and_hash(name, hash);

	//Return the api at the found index
	return &g_apis[index];
}

static struct sl_api_registry registry = {
	set_api,
	remove_api,
	get_api
};

struct sl_api_registry* sl_global_api_registry = &registry;
