#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"

#include "api_versions.h"
#include "version.h"
#include "protocol_body.h"

struct MemberIdentity
{
    std::string m_member_id;
    std::string m_group_instance_id;
};

struct LeaveGroupRequest : protocol_body, flexible_version
{
    int16_t m_version = 0;
    std::string m_group_id;
    std::string m_member_id;
    std::vector<MemberIdentity> m_members;
    LeaveGroupRequest() = default;
    LeaveGroupRequest(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool is_flexible() const;
    bool is_flexible_version(int16_t ver) const;
    KafkaVersion required_version() const;
};
