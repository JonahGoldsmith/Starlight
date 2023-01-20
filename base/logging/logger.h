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


#ifndef LOGGER_H
#define LOGGER_H

#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

enum sl_log_level
{
	sl_log_level_info,

	sl_log_level_debug,

	sl_log_level_error
};

typedef struct sl_logger {
	void *inst;

	void (*log)(struct sl_logger *logger, enum sl_log_level level, const char *message);

} sl_logger;

struct sl_logger_api {

	void (*register_logger)(const sl_logger *logger);

	void (*unregister_logger)(const sl_logger *logger);

	int (*log_printf)(enum sl_log_level level, const char *file, uint32_t line, const char *format, ...);
};

#define SL_LOGGER_API "sl_logger_api"

#define SL_LOG_INFO(format, ...) sl_logger_api->log_printf(sl_log_level_info, __FILE__, __LINE__, "" format "", ##__VA_ARGS__)

#define SL_LOG_DEBUG(format, ...) sl_logger_api->log_printf(sl_log_level_debug, __FILE__, __LINE__, "" format "", ##__VA_ARGS__)

#define SL_LOG_ERROR(format, ...) sl_logger_api->log_printf(sl_log_level_error, __FILE__, __LINE__, "" format "", ##__VA_ARGS__)

#ifdef NOT_IMPLEMENTED_LOG
#define SL_NOT_IMPLEMENTED() sl_logger_api->log_printf(sl_log_level_debug, __FILE__, __LINE__, "%s IS NOT IMPLEMENTED!\n", SL_FUNCTION)
#else
#define SL_NOT_IMPLEMENTED()
#endif

#ifdef LINKS_SL_BASE
extern void init_logger_system(void);
extern struct sl_logger_api *sl_logger_api;
#endif

#ifdef __cplusplus
}
#endif

#endif//LOGGER_H
