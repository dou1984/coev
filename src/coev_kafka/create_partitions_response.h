#pragma once

#include <string>
#include <map>
#include <memory>
#include <cstdint>
#include <chrono>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "metrics.h"
#include "protocol_body.h"
#include "version.h"

struct TopicPartitionError : IEncoder, VDecoder
{
    KError m_err;
    std::string m_err_msg;

    std::string Error() const;
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};

struct CreatePartitionsResponse : protocol_body
{
    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    std::map<std::string, std::shared_ptr<TopicPartitionError>> m_topic_partition_errors;

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
