#pragma once
#include <vector>
#include "encoder_decoder.h"
#include "packet_decoder.h"
#include "packet_encoder.h"

struct RecordHeader : IEncoder, IDecoder
{
    std::string m_key;
    std::string m_value;

    RecordHeader() = default;
    RecordHeader(const std::string &k, const std::string &v);
    int encode(packetEncoder &pe) const;
    int decode(packetDecoder &pd);
};
