#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "api_versions.h"
#include "protocol_body.h"
#include "consumer_group_members.h"

struct GroupProtocol : IDecoder, IEncoder
{

    std::string Name;
    std::string Metadata;

    int decode(PDecoder &pd);
    int encode(PEncoder &pe);
};

struct JoinGroupRequest : protocolBody
{

    int16_t Version = 0;
    std::string GroupId;
    int32_t SessionTimeout = 0;
    int32_t RebalanceTimeout = 0;
    std::string MemberId;
    std::string GroupInstanceId;
    std::string ProtocolType;
    std::map<std::string, std::string> GroupProtocols;
    std::vector<std::shared_ptr<GroupProtocol>> OrderedGroupProtocols;
    JoinGroupRequest() = default;
    JoinGroupRequest(int16_t v) : Version(v)
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
    void AddGroupProtocol(const std::string &name, const std::string &metadata);
    int AddGroupProtocolMetadata(const std::string &name, const std::shared_ptr<ConsumerGroupMemberMetadata> &metadata);
};
