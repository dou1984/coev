#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "version.h"
#include "api_versions.h"
#include "protocol_body.h"

struct GroupData
{
    std::string GroupState;
    std::string GroupType;
};

struct ListGroupsResponse : protocolBody
{

    int16_t Version = 0;
    std::chrono::milliseconds ThrottleTime;
    KError Err;
    std::unordered_map<std::string, std::string> Groups;
    std::unordered_map<std::string, GroupData> GroupsData;

    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version()const;
    int16_t headerVersion()const;
    bool isValidVersion()const;
    bool isFlexible();
    static bool isFlexibleVersion(int16_t ver);
    KafkaVersion requiredVersion()const;
};
