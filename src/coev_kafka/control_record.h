#pragma once

#include <memory>
#include "packet_decoder.h"
#include "packet_encoder.h"
#include "control_record_type.h"

struct ControlRecord
{
    int16_t Version;
    int32_t CoordinatorEpoch;
    ControlRecordType Type;
    ControlRecord() = default;
    ControlRecord(int16_t v, int32_t c, ControlRecordType t)
        : Version(v), CoordinatorEpoch(c), Type(t)
    {
    }
    int decode(PDecoder &key, PDecoder &value);
    int encode(PEncoder &key, PEncoder &value);
};
