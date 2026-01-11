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
    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct SyncGroupRequest : protocol_body
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
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);

    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool isFlexible();
    bool isFlexibleVersion(int16_t ver);
    KafkaVersion required_version() const;

    void AddGroupAssignment(const std::string &memberId, const std::string &memberAssignment);
    int AddGroupAssignmentMember(const std::string &memberId, std::shared_ptr<ConsumerGroupMemberAssignment> memberAssignment);
};
