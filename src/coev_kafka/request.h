#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <cstdint>
#include "version.h"
#include "packet_decoder.h"
#include "packet_encoder.h"
#include "protocol_body.h"
#include "real_decoder.h"

inline constexpr const int32_t MaxRequestSize = 100 * 1024 * 1024;

struct Request : VEncoder, IEncoder, IDecoder
{
    int32_t m_correlation_id = 0;
    std::string_view m_client_id;
    const protocol_body *m_body;
    int encode(packet_encoder &pe) const;
    int encode(packet_encoder &pe, int16_t version) const;
    int decode(packet_decoder &pd);
    bool is_flexible() const;
};
