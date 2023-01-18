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

#include "register_engine_apis.h"

#include "base/registry/api_registry.h"
#include "base/logging/logger.h"
#include "base/memory/allocator.h"
#include "base/memory/mem_tracker.h"
#include "base/os/os.h"
#include "base/registry/plugin_system.h"
#include "base/thread/job_system.h"
#include "base/util/sprintf.h"

//must be called after job system creation
void register_engine_apis(void)
{
	SL_REGISTRY_SET_API(SL_API_REGISTRY_API, sl_global_api_registry);
	SL_REGISTRY_SET_API(SL_LOGGER_API, sl_logger_api);
	SL_REGISTRY_SET_API(SL_ALLOCATOR_API, sl_allocator_api);
	SL_REGISTRY_SET_API(SL_MEM_TRACKER_API, sl_memory_tracker_api);
	SL_REGISTRY_SET_API(SL_OS_API, sl_os_api);
	SL_REGISTRY_SET_API(SL_PLUGIN_SYSTEM_API, sl_plugin_system_api);
	SL_REGISTRY_SET_API(SL_JOB_SYSTEM_API, sl_get_job_system());
	SL_REGISTRY_SET_API(SL_SPRINTF_API, sl_sprintf_api);
}

