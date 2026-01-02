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
    std::string MemberId;
    std::string Assignment;
    SyncGroupRequestAssignment() = default;
    SyncGroupRequestAssignment(const std::string &memberId, const std::string &assignment) : MemberId(memberId), Assignment(assignment)
    {
    }
    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct SyncGroupRequest : protocolBody
{
    int16_t Version = 0;
    std::string GroupId;
    int32_t GenerationId = 0;
    std::string MemberId;
    std::string GroupInstanceId;
    std::vector<SyncGroupRequestAssignment> GroupAssignments;
    SyncGroupRequest() = default;
    SyncGroupRequest(int16_t v) : Version(v)
    {
    }
    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);

    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    bool isFlexible();
    bool isFlexibleVersion(int16_t ver);
    KafkaVersion requiredVersion() const;

    void AddGroupAssignment(const std::string &memberId, const std::string &memberAssignment);
    int AddGroupAssignmentMember(const std::string &memberId, std::shared_ptr<ConsumerGroupMemberAssignment> memberAssignment);
};
