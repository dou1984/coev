#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"

#include "api_versions.h"
#include "protocol_body.h"
#include "version.h"
#include "consumer_group_members.h"

struct SyncGroupRequestAssignment : VDecoder, VEncoder
{
    std::string m_member_id;
    std::string m_assignment;
    SyncGroupRequestAssignment() = default;
    SyncGroupRequestAssignment(const std::string &memberId, const std::string &assignment) : m_member_id(memberId), m_assignment(assignment)
    {
    }
    int encode(packet_encoder &pe, int16_t version) const;
    int decode(packet_decoder &pd, int16_t version);
};

struct SyncGroupRequest : protocol_body, flexible_version
{
    int16_t m_version = 0;
    std::string m_group_id;
    int32_t m_generation_id = 0;
    std::string m_member_id;
    std::string m_group_instance_id;
    std::vector<SyncGroupRequestAssignment> m_group_assignments;
    SyncGroupRequest() = default;
    SyncGroupRequest(int16_t v) : m_version(v)
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

    void add_group_assignment(const std::string &memberId, const std::string &memberAssignment);
    int add_group_assignment_member(const std::string &memberId, std::shared_ptr<ConsumerGroupMemberAssignment> memberAssignment);
};
