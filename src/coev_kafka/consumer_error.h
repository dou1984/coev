#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include "errors.h"

struct ConsumerError
{
    std::string Topic;
    int32_t Partition;
    KError Err;
    ConsumerError() = default;
    ConsumerError(const std::string &topic, int32_t partition, int err) : ConsumerError(topic, partition, KError(err))
    {
    }
    ConsumerError(const std::string &topic, int32_t partition, KError err)
        : Topic(topic), Partition(partition), Err(err)
    {
    }
    std::string Error() const
    {
        return KErrorToString(Err);
    }
};

using ConsumerErrors = std::vector<std::shared_ptr<ConsumerError>>;