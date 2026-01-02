#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "api_versions.h"
#include "version.h"
#include "errors.h"
#include "consumer_group_members.h"
#include "balance_strategy.h"
#include "describe_groups_response.h"
#include "protocol_body.h"

struct GroupMemberDescription : VEncoder, VDecoder
{

    int16_t Version;
    std::string MemberId;
    std::string GroupInstanceId;
    std::string ClientId;
    std::string ClientHost;
    std::string MemberMetadata;
    std::string MemberAssignment;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
    std::shared_ptr<ConsumerGroupMemberAssignment> GetMemberAssignment();
    std::shared_ptr<ConsumerGroupMemberMetadata> GetMemberMetadata();
};

struct GroupDescription : VEncoder, VDecoder
{
    int16_t Version;
    KError Err;
    int16_t ErrorCode;
    std::string GroupId;
    std::string State;
    std::string ProtocolType;
    std::string Protocol;
    std::map<std::string, std::shared_ptr<GroupMemberDescription>> Members;
    int32_t AuthorizedOperations;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct DescribeGroupsResponse : protocolBody
{
    int16_t Version;
    std::chrono::milliseconds ThrottleTime;
    std::vector<std::shared_ptr<GroupDescription>> Groups;

    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    bool isFlexible() const;
    static bool isFlexibleVersion(int16_t version);
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;
};
