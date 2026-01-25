#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include "version.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "record_header.h"
#include "variant_length_field.h"

const int8_t isTransactionalMask = 0x10;
const int8_t controlMask = 0x20;

struct Record : IEncoder, IDecoder
{
    std::vector<std::shared_ptr<RecordHeader>> m_headers;
    int8_t m_attributes;
    std::chrono::milliseconds m_timestamp_delta;
    int64_t m_offset_delta;
    std::string m_key;
    std::string m_value;
    std::shared_ptr<VariantLengthField> m_length = std::make_shared<VariantLengthField>();

    Record() = default;

    Record(std::string &key, std::string &value, int64_t offsetDelta, std::chrono::milliseconds timestampDelta) : m_key(key), m_value(value), m_offset_delta(offsetDelta), m_timestamp_delta(timestampDelta)
    {
    }
    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd);
};
