#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cstdint>
#include <chrono>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "version.h"
#include "api_versions.h"
#include "consumer_group_members.h"
#include "protocol_body.h"

struct GroupMember
{

    std::string m_member_id;
    std::string m_group_instance_id;
    std::string m_metadata;
};

struct JoinGroupResponse : protocol_body, flexible_version
{

    int16_t m_version = 0;
    std::chrono::milliseconds m_throttle_time;
    KError m_err;
    int32_t m_generation_id = 0;
    std::string m_group_protocol;
    std::string m_leader_id;
    std::string m_member_id;
    std::vector<GroupMember> m_members;

    void set_version(int16_t v);
    int encode(packetEncoder &pe) const;
    int decode(packetDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool is_flexible() const;
    bool is_flexible_version(int16_t ver) const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
    int get_members(std::map<std::string, ConsumerGroupMemberMetadata> &members);
};
