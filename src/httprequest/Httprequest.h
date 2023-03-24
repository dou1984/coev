/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <http_parser.h>
#include "../coev.h"

namespace coev
{
	struct Httprequest : http_parser, EVRecv
	{
		Httprequest();
		virtual ~Httprequest() = default;
		int parse(const char*, int);	
	};
}