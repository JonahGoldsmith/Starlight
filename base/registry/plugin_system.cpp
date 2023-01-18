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


#include "plugin_system.h"
#include "logging/logger.h"
#include "memory/allocator.h"
#include "util/sprintf.h"
#include "api_registry.h"

extern "C" struct sl_logger_api* sl_logger_api;
extern "C" struct sl_allocator_api* sl_allocator_api;
extern "C" struct sl_sprintf_api* sl_sprintf_api;
extern "C" struct sl_api_registry* sl_global_api_registry;

#include <stdio.h>

#ifndef NDEBUG
#define CR_DEBUG
#endif

//TODO: FILE SYSTEM
static FILE* CodeReloadLog;

static sl_allocator plugin_system_allocator;

#define CR_LOG(...) fprintf(CodeReloadLog, __VA_ARGS__); fflush(CodeReloadLog)
#define CR_ERROR(...) SL_LOG_ERROR(__VA_ARGS__)

#ifdef CR_DEBUG
#   define CR_TRACE CR_LOG("CR_TRACE: %s\n", __FUNCTION__);
#else
#   define CR_TRACE
#endif

#ifdef _MSC_VER
#include <assert.h>
#endif
#define CR_ASSERT(b) assert(b)
#define CR_REALLOC(ptr, size) sl_realloc(&plugin_system_allocator, ptr, size)
#define CR_FREE(ptr) sl_free(&plugin_system_allocator, ptr);
#define CR_MALLOC(size) sl_alloc(&plugin_system_allocator, size)
#define CR_STRLEN(x) (x ? strlen(x) : 0)

#define CR_HOST CR_SAFE
#include "third-party/cr/cr.h"


#if SL_PLATFORM_WINDOWS
#include "third-party/dirent_msvc/dirent.h"
#include "direct.h"
#define LIB_EXTENSION ".dll"
#define PATH_SLASH "\\"
#elif SL_PLATFORM_OSL
#include <dirent.h>
#define _mkdir(x) mkdir(x, ACCESSPERMS)
#define LIB_EXTENSION ".dylib"
#define PATH_SLASH "/"
#endif

static cr_plugin plugins[128];

static uint32_t numPlugins = 0;

static void search(const char* path, const char* tempPath) {
	DIR* dir = opendir(path);

	_mkdir(tempPath);

	if (dir) {
		struct dirent* ent;
		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_type == DT_DIR) {
				// Found a directory, but ignore . and ..
				if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
					char subdir[512];
					sl_snprintf(subdir, sizeof(subdir), "%s%s%s", path, PATH_SLASH, ent->d_name);
					search(subdir, tempPath);
				}
			}
			else {
				// Found a file, check if it has the .dll extension
				if (strstr(ent->d_name, LIB_EXTENSION) != NULL) {
					char subdir[512];
					sl_snprintf(subdir, sizeof(subdir), "%s%s%s", path, PATH_SLASH, ent->d_name);
					cr_plugin_open(&plugins[numPlugins], subdir);
					cr_set_temporary_path(&plugins[numPlugins], tempPath);
					numPlugins++;
					//printf("Found %s\n", subdir);
				}
			}
		}

		closedir(dir);
	}
}

static void load_plugins(const char* path, const char* tempPath)
{
	CodeReloadLog = fopen("CodeReloadLog.txt", "w");
	search(path, tempPath);
}

void hot_reload_plugins()
{
	for(int i = 0; i < numPlugins; i++)
	{
		cr_plugin_update(&plugins[i]);
	}
}

void unload_plugins()
{
	// at the end do not forget to cleanup the plugin context, as it needs to
	// allocate some memory to track internal and plugin states
	for(int i = 0; i < numPlugins; i++)
	{
		cr_plugin_close(&plugins[i]);
	}

	fclose(CodeReloadLog);

}

static sl_plugin_system_api plugin_api = {
	load_plugins,
	hot_reload_plugins,
	unload_plugins,
};

extern "C" struct sl_plugin_system_api* sl_plugin_system_api = &plugin_api;

extern "C" void sl_init_plugin_system(void)
{
	plugin_system_allocator = sl_allocator_api->create_child(sl_allocator_api->system, "plugin_system");
}

extern "C" void sl_shutdown_plugin_system(void)
{
	unload_plugins();
	sl_allocator_api->destroy_child(&plugin_system_allocator);
}


