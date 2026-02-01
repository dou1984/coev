#pragma once

#include <memory>
#include <string>

#include "packet_decoder.h"
#include "packet_encoder.h"
#include "errors.h"
#include "version.h"
#include "protocol_body.h"

struct Broker;
struct ConsumerMetadataResponse : protocol_body
{

    int16_t m_version;
    KError m_err;
    mutable std::shared_ptr<Broker> m_coordinator;
    int32_t m_coordinator_id;
    std::string m_coordinator_host;
    int32_t m_coordinator_port;
    ConsumerMetadataResponse() = default;
    ConsumerMetadataResponse(int16_t v);
    void set_version(int16_t v);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    int decode(packet_decoder &pd, int16_t version);
    int encode(packet_encoder &pe) const;
};
