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

#ifndef PLUGIN_SYSTEM_H
#define PLUGIN_SYSTEM_H

#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interface of the plugin System
 * Loading, Unloading, and HotReloading plugins/Plugins
 */
struct sl_plugin_system_api {
	/**
 * @brief Loads All Shared Libs at a Specified path
 * @param base_load_path Path to be Loaded From
 * @param temp_path Path for Temporary Files to be Copied too
 */
	void (*load_all_plugins)(const char *base_load_path, const char *temp_path);

	/**
 * @brief Checks All plugins for Changes and Hot Reloads If Necessary
 */
	void (*check_hot_reload)(void);

	/**
 * @brief Unloads and Frees all plugins
 */
	void (*unload_all_plugins)(void);
};

#define SL_PLUGIN_SYSTEM_API "sl_plugin_system_api"

#ifdef LINKS_SL_BASE

extern struct sl_plugin_system_api* sl_plugin_system_api;

extern void sl_init_plugin_system(void);

extern void sl_shutdown_plugin_system(void);

#endif


#ifdef __cplusplus
}
#endif

#endif//PLUGIN_SYSTEM_H
