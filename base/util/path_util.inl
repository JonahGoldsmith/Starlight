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

#ifndef PATH_UTIL_INL
#define PATH_UTIL_INL

#include "defines.h"

// Returns the part of the path after the last / or \ (if any).
SL_FORCE_INLINE const char* sl_get_file_name(const char* path)
{
	for (const char* ptr = path; *ptr != '\0'; ++ptr)
	{
		if (*ptr == '/' || *ptr == '\\')
		{
			path = ptr + 1;
		}
	}
	return path;
}

//Referenced https://stackoverflow.com/questions/2328182/prepending-to-a-string
/* Prepends t into s. Assumes s has enough space allocated
** for the combined string.
 * TODO: Is this a slow operation? Does it matter?
*/
SL_FORCE_INLINE void sl_prepend_string(char* s, const char* t)
{
	size_t len = strlen(t);
	sl_memmove(s + len, s, strlen(s) + 1);
	sl_memcpy(s, t, len);
}

SL_FORCE_INLINE void sl_get_two_dirs_back(char *exec_path, char *result) {
	char *last_slash = NULL;
	int count = 0;

	if (exec_path == NULL || strlen(exec_path) == 0 || result == NULL) {
		return;
	}

	strcpy(result, exec_path);

	// Find the last slash in the path
#ifdef _WIN32
	last_slash = strrchr(result, '\\');
#else
	last_slash = strrchr(result, '/');
#endif
	if (last_slash == NULL) {
		return;
	}

	// Go back two directories by replacing the last two slashes with null characters
	while (count < 2 && last_slash != NULL) {
		*last_slash = '\0';
#ifdef _WIN32
		last_slash = strrchr(result, '\\');
#else
		last_slash = strrchr(result, '/');
#endif
		count++;
	}
}

SL_FORCE_INLINE void sl_get_one_dir_back(char *exec_path, char *result) {
	char *last_slash = NULL;
	int count = 0;

	if (exec_path == NULL || strlen(exec_path) == 0 || result == NULL) {
		return;
	}

	strcpy(result, exec_path);

	// Find the last slash in the path
#ifdef _WIN32
	last_slash = strrchr(result, '\\');
#else
	last_slash = strrchr(result, '/');
#endif
	if (last_slash == NULL) {
		return;
	}

	// Go back two directories by replacing the last two slashes with null characters
	while (count < 1 && last_slash != NULL) {
		*last_slash = '\0';
#ifdef _WIN32
		last_slash = strrchr(result, '\\');
#else
		last_slash = strrchr(result, '/');
#endif
		count++;
	}
}

SL_FORCE_INLINE void sl_concat_dir(char *dir_name, char *path, char *result) {
	size_t dir_name_len = strlen(dir_name);
	size_t path_len = strlen(path);

	if (result == NULL) {
		return;
	}

	strcpy(result, path);
#ifdef _WIN32
	result[path_len] = '\\';  // add '\\' between path and dir_name
#else
	result[path_len] = '/';  // add '/' between path and dir_name
#endif
	strcpy(result + path_len + 1, dir_name);  // concatenate dir_name to the end of path

}

SL_FORCE_INLINE void sl_concat_dir_end_slash(char *dir_name, char *path, char *result) {
	size_t dir_name_len = strlen(dir_name);
	size_t path_len = strlen(path);

	if (result == NULL) {
		return;
	}

	strcpy(result, path);
#ifdef _WIN32
	result[path_len] = '\\';  // add '\\' between path and dir_name
#else
	result[path_len] = '/';  // add '/' between path and dir_name
#endif
	strcpy(result + path_len + 1, dir_name);  // concatenate dir_name to the end of path

#ifdef _WIN32
	result[path_len + dir_name_len + 1] = '\\';
#else
	result[path_len + dir_name_len + 1] = '/';
#endif

}

#if SL_PLATFORM_OSL
#include <mach-o/dyld.h>
#include <stdlib.h>
#elif SL_PLATFORM_WINDOWS
#include <windows.h>
#endif

SL_FORCE_INLINE void sl_get_executable_path(char* buffer, uint32_t size)
{
#if defined (__APPLE__)
	if (_NSGetExecutablePath(buffer, &size) != 0) {
		buffer[0] = '\0'; // buffer too small (!)
	} else {
		// resolve symlinks, ., .. if possible
		char *canonicalPath = realpath(buffer, NULL);
		if (canonicalPath != NULL) {
			strncpy(buffer,canonicalPath,size);
		}
	}
#elif defined(_WIN32)
	GetModuleFileNameA(0, buffer, size - 1);
#endif
}

#endif//PATH_UTIL_INL
