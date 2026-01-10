#pragma once

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"
#include "topic_partition.h"

struct CreatePartitionsRequest : protocol_body, throttle_support
{
    int16_t m_version;
    std::map<std::string, std::shared_ptr<TopicPartition>> m_topic_partitions;
    std::chrono::milliseconds m_timeout;
    bool m_validate_only;

    CreatePartitionsRequest() = default;
    CreatePartitionsRequest(int16_t v) : m_version(v)
    {
    }

    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttleTime() const;
};
