#pragma once
#include <vector>
#include <cstdint>
#include <chrono>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "api_versions.h"
#include "errors.h"
#include "consumer_group_members.h"
#include "protocol_body.h"

struct SyncGroupResponse : protocolBody
{

    int16_t Version = 0;
    std::chrono::milliseconds ThrottleTime;
    KError Err = ErrNoError;
    std::string MemberAssignment;

    void setVersion(int16_t v);

    int GetMemberAssignment(std::shared_ptr<ConsumerGroupMemberAssignment> &);

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);

    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    bool isFlexible();
    bool isFlexibleVersion(int16_t ver);
    KafkaVersion requiredVersion() const;

    std::chrono::milliseconds throttleTime() const;
};
