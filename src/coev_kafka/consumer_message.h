#pragma once
#include <vector>
#include <chrono>
#include <memory>
#include "record_header.h"

struct ConsumerMessage
{
    ConsumerMessage() = default;
    auto &Key() { return m_key; }
    auto &Value() { return m_value; }

    std::vector<std::shared_ptr<RecordHeader>> m_headers;
    std::chrono::system_clock::time_point m_timestamp;
    std::chrono::system_clock::time_point m_block_timestamp;

    std::string m_key;
    std::string m_value;
    std::string m_topic;
    int32_t m_partition;
    int64_t m_offset;
};