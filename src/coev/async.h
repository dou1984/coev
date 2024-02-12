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
	struct fmutex
	{
		void lock() {}
		void unlock() {}
	};
	struct event_list : chain
	{
	};
	struct event_list_mutex : chain, std::mutex
	{
	};
	using evl = event_list;
	using evlts = event_list_mutex;

	template <typename T>
	concept async_t = std::is_same<T, evl>::value || std::is_same<T, evlts>::value;

	template <async_t G = evl, async_t... T>
	struct async : std::tuple<G, T...>
	{
		template <size_t I = 0>
		chain *data()
		{
			return static_cast<chain *>(&std::get<I>(*this));
		}
	};

}