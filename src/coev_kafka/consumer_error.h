/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include "errors.h"

namespace coev::kafka
{

    struct ConsumerError
    {
        std::string m_topic;
        int32_t m_partition;
        KError m_err;
        ConsumerError() = default;
        ConsumerError(const std::string &topic, int32_t partition, int err) : ConsumerError(topic, partition, KError(err))
        {
        }
        ConsumerError(const std::string &topic, int32_t partition, KError err)
            : m_topic(topic), m_partition(partition), m_err(err)
        {
        }
        std::string Error() const
        {
            return KErrorToString(m_err);
        }
    };

} // namespace coev::kafka
