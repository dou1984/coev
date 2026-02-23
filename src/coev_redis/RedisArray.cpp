/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include "RedisArray.h"
namespace coev
{
    RedisArray::RedisArray(redisReply **element, size_t elements) : m_element(element), m_elements(elements)
	{
		if (m_elements > INT64_MAX)
		{
			throw("RedisArray construct m_elements bigger than INT64_MAX");
		}
	}
}