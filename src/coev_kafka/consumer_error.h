#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include "errors.h"

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

using ConsumerErrors = std::vector<std::shared_ptr<ConsumerError>>;