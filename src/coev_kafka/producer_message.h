#pragma once

#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <cstdint>
#include <variant>

#include "utils.h"
#include "record.h"
#include "errors.h"
#include "interceptors.h"

inline constexpr int producerMessageOverhead = 26;
inline constexpr int maximumRecordOverhead = 77; // 5 * 5 + 10 + 1; // binary.MaxVarintLen32=5, binary.MaxVarintLen64=10

enum FlagSet : int16_t
{
    Syn = 1 << 0,
    Fin = 1 << 1,
    Shutdown = 1 << 2,
    Endtxn = 1 << 3,
    Committxn = 1 << 4,
    Aborttxn = 1 << 5
};

struct ProducerMessage : std::enable_shared_from_this<ProducerMessage>
{
    std::string Topic;
    std::shared_ptr<Encoder> Key;
    std::shared_ptr<Encoder> Value;
    std::vector<RecordHeader> Headers;
    std::shared_ptr<void> Metadata;

    int64_t Offset;
    int32_t Partition;
    std::chrono::system_clock::time_point Timestamp;

    int Retries;
    FlagSet Flags;
    int32_t SequenceNumber;
    int16_t ProducerEpoch;
    bool hasSequence;

    ProducerMessage();
    int ByteSize(int version) const;
    void Clear();
};
