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
//
// Created by Jonah Goldsmith on 1/3/23.
//
#include "os_window.h"

#if SL_PLATFORM_OSL

#include "base/logging/logger.h"
#include "stdio.h"
#include "base/registry/api_registry.h"
#include "base/third-party/cr/cr.h"
#include "base/logging/logger.h"

static struct sl_logger_api* sl_logger_api;

CR_EXPORT int cr_main(struct sl_api_registry* reg, struct cr_plugin *ctx, enum cr_op operation)
{

	switch(operation) {
	case CR_LOAD:
		sl_logger_api = reg->get(SL_LOGGER_API);
		SL_LOG_INFO("Testing Window Plugin!\n");
		return 0;
		break;
	case CR_UNLOAD:
		return 0;
		break;
	}

	return 0;

}

#endif