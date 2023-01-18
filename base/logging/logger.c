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

#include "logger.h"

#include "util/sprintf.h"
#include "util/path_util.inl"
#include "os/os.h"
#include <stdio.h>
#include <sys/time.h>

#define MAX_LOGGERS 24

static uint32_t num_loggers;
static sl_logger loggers[MAX_LOGGERS];

#if SL_PLATFORM_WINDOWS
void OutputDebugStringA(const char *s);
#endif

extern struct sl_sprintf_api* sl_sprintf_api; //sprintf.c
extern struct sl_os_api* sl_os_api;

void default_print(struct sl_logger* logger, enum sl_log_level level, const char* message)
{
	fputs(message, stdout);

#if SL_PLATFORM_WINDOWS
	OutputDebugStringA(message);
#endif

}

static sl_logger default_logger = {
	.log = default_print
};

void init_logger_system(void)
{
		num_loggers = 1;
		loggers[0] = default_logger;
}

void register_logger(const sl_logger* logger)
{
	for (uint32_t i = 0; i < num_loggers; ++i) {
		if (sl_memcmp(loggers + i, logger, sizeof(sl_logger)) == 0)
			return;
	}

	if (num_loggers >= MAX_LOGGERS) {
		//TODO ERROR MESSAGE!
		return;
	}

	loggers[num_loggers] = *logger;
	num_loggers += 1;
}

void unregister_logger(const sl_logger* logger)
{
	for (uint32_t i = 0; i < num_loggers; ++i) {
		if (sl_memcmp(loggers + i, logger, sizeof(sl_logger)) == 0) {
			num_loggers -= 1;
			loggers[i] = loggers[num_loggers];
			return;
		}
	}
}

static void log_print(enum sl_log_level level, const char *message)
{
	for (uint32_t i = 0; i < num_loggers; i++) {
		loggers[i].log(loggers[i].inst, level, message);
	}
}

static int log_printf(enum sl_log_level level, const char* file, uint32_t line, const char *format, ...)
{

	char buffer[2400];
	sl_memset(buffer, 0, sizeof(char)*2400);
	__builtin_va_list arg_ptr;
	va_start(arg_ptr, format);
	int res = sl_vsprintf(buffer, format, arg_ptr);
	va_end(arg_ptr);

	const char* level_strings[3] = {"[INFO]: ", "[DEBUG]: ", "[ERROR]: "};
	char prologue[8000];
	sl_memset(prologue, 0, sizeof(char)*8000);
	time_t rawtime;
	struct tm * timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	char thread_name[64];
	sl_os_api->thread->get_thread_name(thread_name, 64);

	sl_sprintf(prologue, "[%d-%d-%d] %s:%d [%s] %s%s",  timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_year + 1900, sl_get_file_name(file), line, thread_name, level_strings[level], buffer);

	log_print(level, prologue);
	return res;
}

static struct sl_logger_api logger_api = {
	.log_printf = log_printf,
	.register_logger = register_logger,
	 .unregister_logger = unregister_logger,
};

struct sl_logger_api* sl_logger_api = &logger_api;