#pragma once

#include <string>
#include <map>
#include <vector>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "protocol_body.h"

struct DeleteRecordsResponsePartition : IEncoder, versioned_encoder
{
    int64_t m_low_watermark;
    KError m_err;

    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);
};

struct DeleteRecordsResponseTopic : versioned_decoder, IEncoder
{
    std::map<int32_t, std::shared_ptr<DeleteRecordsResponsePartition>> m_partitions;

    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);

    ~DeleteRecordsResponseTopic();
};

struct DeleteRecordsResponse : protocol_body
{
    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    std::map<std::string, std::shared_ptr<DeleteRecordsResponseTopic>> m_topics;

    ~DeleteRecordsResponse();
    void set_version(int16_t v);
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
};
