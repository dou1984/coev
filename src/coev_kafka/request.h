#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include "version.h"
#include "packet_decoder.h"
#include "packet_encoder.h"
#include "protocol_body.h"
#include "real_decoder.h"

struct Broker;

inline constexpr const int32_t MaxRequestSize = 100 * 1024 * 1024;

struct request : IEncoder, IDecoder
{
    int32_t correlationID;
    std::string clientID;
    std::shared_ptr<protocolBody> body;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd);
};

coev::awaitable<int> decodeRequest(std::shared_ptr<Broker> &broker, int &req, request &size);
std::shared_ptr<protocolBody> allocateBody(int16_t key, int16_t version);
