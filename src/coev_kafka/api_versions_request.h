#pragma once

#include <string>
#include <cstdint>
#include "version.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"

inline const std::string defaultClientSoftwareName = "coev";

struct ApiVersionsRequest : protocolBody
{
    int16_t Version = 0;
    std::string ClientSoftwareName;
    std::string ClientSoftwareVersion;
    ApiVersionsRequest() = default;
    ApiVersionsRequest(int16_t v) : Version(v)
    {
        ClientSoftwareName = defaultClientSoftwareName;
    }
    void setVersion(int16_t v);

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);

    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    bool isFlexible() const;
    bool isFlexibleVersion(int16_t version) const;
    KafkaVersion requiredVersion() const;
};
