/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#pragma once

namespace coev
{
	template <class T>
	void __setvalue(T &value, const char *v)
	{
		char *end = nullptr;
		if constexpr (std::is_same<T, std::string>::value)
			value = v ? v : "";
		else if constexpr (std::is_same<T, const char *>::value)
			value = v;
		else if constexpr (std::is_integral<T>::value)
			value = std::strtoll(v, &end, 10);
		else if constexpr (std::is_floating_point<T>::value)
			value = std::strtod(v, &end);
		else
			throw("setvalue type is error");
	}
}