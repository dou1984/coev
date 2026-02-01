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

struct Request : VEncoder, IEncoder, IDecoder
{
    int32_t m_correlation_id = 0;
    std::string m_client_id;
    const protocol_body *m_body;
    int encode(packet_encoder &pe) const;
    int encode(packet_encoder &pe, int16_t version) const
    {
        return encode(pe);
    }
    int decode(packet_decoder &pd);
    bool is_flexible() const;
};

coev::awaitable<int> decodeRequest(std::shared_ptr<Broker> &broker, int &req, Request &size);
std::shared_ptr<protocol_body> allocateBody(int16_t key, int16_t version);
