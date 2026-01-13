#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "api_versions.h"
#include "protocol_body.h"

struct InitProducerIDRequest : protocol_body
{

    int16_t m_version = 0;
    std::string m_transactional_id;
    std::chrono::milliseconds m_transaction_timeout;
    int64_t m_producer_id = 0;
    int16_t m_producer_epoch = 0;
    InitProducerIDRequest() = default;
    InitProducerIDRequest(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);
    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool is_flexible();
    static bool is_flexible_version(int16_t ver);
    KafkaVersion required_version() const;
};
