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

    std::string MemberId;
    std::string GroupInstanceId;
    std::string Metadata;
};

struct JoinGroupResponse : protocolBody
{

    int16_t Version = 0;
    std::chrono::milliseconds ThrottleTime;
    KError Err;
    int32_t GenerationId = 0;
    std::string GroupProtocol;
    std::string LeaderId;
    std::string MemberId;
    std::vector<GroupMember> Members;

    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    bool isFlexible();
    static bool isFlexibleVersion(int16_t ver);
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;
    int GetMembers(std::map<std::string, ConsumerGroupMemberMetadata> &members);
};
