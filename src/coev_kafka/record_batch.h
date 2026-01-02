#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <chrono>
#include <string>

#include "record.h"
#include "message.h"
#include "version.h"
#include "config.h"
const int RECORD_BATCH_OVERHEAD = 49;

struct RecordBatch
{

    int64_t FirstOffset = 0;
    int32_t PartitionLeaderEpoch = 0;
    int8_t Version = 2;
    CompressionCodec Codec = CompressionCodec::None;
    int CompressionLevel = 0;
    bool Control = false;
    bool LogAppendTime = false;
    int32_t LastOffsetDelta = 0;
    std::chrono::system_clock::time_point FirstTimestamp;
    std::chrono::system_clock::time_point MaxTimestamp;
    int64_t ProducerID = -1;
    int16_t ProducerEpoch = -1;
    int32_t FirstSequence = -1;
    std::vector<std::shared_ptr<Record>> Records;
    bool PartialTrailingRecord = false;
    bool IsTransactional = false;

    std::string compressedRecords;
    size_t recordsLen = 0;

    RecordBatch() = default;
    RecordBatch(int8_t v);
    RecordBatch(int8_t v, bool, std::chrono::system_clock::time_point &first, std::chrono::system_clock::time_point &max);
    int64_t LastOffset() const
    {
        return FirstOffset + static_cast<int64_t>(LastOffsetDelta);
    }
    void AddRecord(std::shared_ptr<Record> r)
    {
        Records.emplace_back(std::move(r));
    }

    int encode(PEncoder &pe);
    int decode(PDecoder &pd);

    int16_t ComputeAttributes() const;
    void EncodeRecords(PEncoder &pe);
};
