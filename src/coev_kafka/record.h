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
    std::vector<std::shared_ptr<RecordHeader>> Headers;
    int8_t Attributes;
    std::chrono::milliseconds TimestampDelta;
    int64_t OffsetDelta;
    std::string Key;
    std::string Value;
    std::shared_ptr<VariantLengthField> length;

    Record() = default;

    Record(std::string &key, std::string &value, int64_t offsetDelta, std::chrono::milliseconds timestampDelta) : Key(key), Value(value), OffsetDelta(offsetDelta), TimestampDelta(timestampDelta)
    {
    }
    int encode(PEncoder &pe);
    int decode(PDecoder &pd);
};
