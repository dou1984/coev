/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

#include <memory>
#include "packet_decoder.h"
#include "packet_encoder.h"
#include "control_record_type.h"

namespace coev::kafka
{
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
        int decode(PacketDecoder &key, PacketDecoder &value);
        int encode(PacketEncoder &key, PacketEncoder &value);
    };
}