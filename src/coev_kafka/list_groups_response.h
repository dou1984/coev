#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "version.h"
#include "api_versions.h"
#include "protocol_body.h"

struct GroupData
{
    std::string m_group_state;
    std::string m_group_type;
};

struct ListGroupsResponse : protocol_body
{

    int16_t m_version = 0;
    std::chrono::milliseconds m_throttle_time;
    KError m_err;
    std::unordered_map<std::string, std::string> m_groups;
    std::unordered_map<std::string, GroupData> m_groups_data;

    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version()const;
    int16_t header_version()const;
    bool is_valid_version()const;
    bool is_flexible();
    static bool is_flexible_version(int16_t ver);
    KafkaVersion required_version()const;
};
