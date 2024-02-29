/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <mutex>
#include <tuple>
#include "chain.h"

namespace coev
{
	struct evl : chain
	{
	};
	struct evlts : chain, std::mutex
	{
	};
	template <typename T>
	concept async_t = std::is_same<T, evl>::value || std::is_same<T, evlts>::value;
	template <async_t G = evl, async_t... T>
	struct async : std::tuple<G, T...>
	{
	};

}