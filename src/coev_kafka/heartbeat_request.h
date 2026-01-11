#pragma once

#include <string>
#include <memory>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "api_versions.h"
#include "protocol_body.h"

struct HeartbeatRequest : protocol_body
{

    int16_t m_version = 0;
    std::string m_group_id;
    int32_t m_generation_id = 0;
    std::string m_member_id;
    std::string m_group_instance_id;
    HeartbeatRequest() = default;
    HeartbeatRequest(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool is_flexible();
    static bool is_flexible_version(int16_t ver);
    KafkaVersion required_version() const;
};
