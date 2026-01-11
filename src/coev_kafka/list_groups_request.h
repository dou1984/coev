#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"

#include "api_versions.h"
#include "version.h"
#include "protocol_body.h"

struct ListGroupsRequest : protocol_body
{

    int16_t m_version = 0;
    std::vector<std::string> m_states_filter;
    std::vector<std::string> m_types_filter;

    ListGroupsRequest() = default;
    ListGroupsRequest(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool isFlexible();
    static bool isFlexibleVersion(int16_t ver);
    KafkaVersion required_version() const;
};
