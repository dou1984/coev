#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"

struct DeleteTopicsRequest : protocol_body
{
    int16_t m_version;
    std::vector<std::string> m_topics;
    std::chrono::milliseconds m_timeout;
    DeleteTopicsRequest() = default;
    DeleteTopicsRequest(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_flexible() const;
    static bool is_flexible_version(int16_t version);
    bool is_valid_version() const;
    KafkaVersion required_version() const;
};
std::shared_ptr<DeleteTopicsRequest> NewDeleteTopicsRequest(KafkaVersion, const std::vector<std::string> &topics, int64_t timeoutMs);