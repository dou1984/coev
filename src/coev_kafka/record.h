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
    std::vector<RecordHeader> m_headers;
    int8_t m_attributes;
    std::chrono::milliseconds m_timestamp_delta;
    int64_t m_offset_delta;
    std::string m_key;
    std::string m_value;
    mutable VariantLengthField m_length;

    Record() = default;
    Record(const std::string &key, const std::string &value, int64_t offset_delta, std::chrono::milliseconds timestamp_delta);
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd);
};
