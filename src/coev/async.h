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

	template <bool mtx>
	struct event_list : chain, std::enable_if<mtx, std::mutex>
	{
		using base = typename std::enable_if<mtx, std::mutex>;
		auto &M()
		{
			if constexpr (mtx)
			{
				return base::type;
			}
			static_assert(false);
		}
	};

	using evl = event_list<false>;
	using evlts = event_list<true>;

	template <typename T>
	concept async_t = std::is_same<T, evl>::value || std::is_same<T, evlts>::value;

	template <async_t G = evl, async_t... T>
	struct async : std::tuple<G, T...>
	{
	};
}