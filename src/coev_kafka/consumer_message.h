#pragma once
#include <vector>
#include <chrono>
#include <memory>
#include "record_header.h"

struct ConsumerMessage
{
    std::vector<std::shared_ptr<RecordHeader>> Headers;
    std::chrono::system_clock::time_point Timestamp;
    std::chrono::system_clock::time_point BlockTimestamp;

    std::string Key;
    std::string Value;
    std::string Topic;
    int32_t Partition;
    int64_t Offset;
    ConsumerMessage() = default;
};