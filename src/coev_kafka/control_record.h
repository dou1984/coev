#pragma once

#include <memory>
#include "packet_decoder.h"
#include "packet_encoder.h"
#include "control_record_type.h"

struct ControlRecord
{
    int16_t m_version;
    int32_t m_coordinator_epoch;
    ControlRecordType m_type;
    ControlRecord() = default;
    ControlRecord(int16_t v, int32_t c, ControlRecordType t)
        : m_version(v), m_coordinator_epoch(c), m_type(t)
    {
    }
    int decode(packet_decoder &key, packet_decoder &value);
    int encode(packet_encoder &key, packet_encoder &value);
};
