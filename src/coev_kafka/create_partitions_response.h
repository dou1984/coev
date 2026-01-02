#pragma once

#include <string>
#include <map>
#include <memory>
#include <cstdint>
#include <chrono>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "metrics.h"
#include "protocol_body.h"
#include "version.h"

struct TopicPartitionError : IEncoder, VDecoder
{
    KError Err;
    std::string ErrMsg;

    std::string Error() const;
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};

struct CreatePartitionsResponse : protocolBody
{
    int16_t Version;
    std::chrono::milliseconds ThrottleTime;
    std::map<std::string, std::shared_ptr<TopicPartitionError>> TopicPartitionErrors;

    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;
};
