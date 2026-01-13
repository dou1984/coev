#pragma once

#include <map>
#include <string>
#include <memory>
#include <chrono>
#include "version.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "protocol_body.h"

struct alterPartitionReassignmentsErrorBlock : IEncoder, IDecoder
{
    KError m_error_code;
    std::string m_error_message;

    alterPartitionReassignmentsErrorBlock() = default;
    alterPartitionReassignmentsErrorBlock(KError err, const std::string &msg) : m_error_code(err), m_error_message(msg)
    {
    }
    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd);
};

struct AlterPartitionReassignmentsResponse : protocol_body
{
    int16_t m_version = 0;
    std::chrono::milliseconds m_throttle_time;
    KError m_error_code = ErrNoError;
    std::string m_error_message;
    std::map<std::string, std::map<int32_t, std::shared_ptr<alterPartitionReassignmentsErrorBlock>>> m_errors;

    AlterPartitionReassignmentsResponse() = default;
    AlterPartitionReassignmentsResponse(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);
    void AddError(const std::string &topic, int32_t partition, KError kerror, std::string message);

    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd, int16_t version);

    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool is_flexible() const;
    bool is_flexible_version(int16_t version) const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
};
