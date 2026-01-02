#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"

struct DeleteGroupsRequest : protocolBody
{
    int16_t Version;
    std::vector<std::string> Groups;
    DeleteGroupsRequest() = default;

    DeleteGroupsRequest(int16_t v) : Version(v)
    {
    }
    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isFlexible() const;
    static bool isFlexibleVersion(int16_t version);
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
    void AddGroup(const std::string &group);
};
