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
    std::string MemberId;
    std::string GroupInstanceId;
};

struct LeaveGroupRequest : protocolBody
{
    int16_t Version = 0;
    std::string GroupId;
    std::string MemberId;
    std::vector<MemberIdentity> Members;
    LeaveGroupRequest() = default;
    LeaveGroupRequest(int16_t v) : Version(v)
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
    static bool isFlexibleVersion(int16_t ver);
    KafkaVersion requiredVersion() const;
};
