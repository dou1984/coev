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

struct CreatePartitionsRequest : protocolBody, throttleSupport
{
    int16_t Version;
    std::map<std::string, std::shared_ptr<TopicPartition>> TopicPartitions;
    std::chrono::milliseconds Timeout;
    bool ValidateOnly;

    CreatePartitionsRequest() = default;
    CreatePartitionsRequest(int16_t v) : Version(v)
    {
    }

    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;
};
