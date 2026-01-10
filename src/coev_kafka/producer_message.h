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

inline constexpr int ProducerMessageOverhead = 26;
inline constexpr int MaximumRecordOverhead = 77; // 5 * 5 + 10 + 1; // binary.MaxVarintLen32=5, binary.MaxVarintLen64=10

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
    std::string m_topic;
    std::shared_ptr<Encoder> m_key;
    std::shared_ptr<Encoder> m_value;
    std::vector<RecordHeader> m_headers;
    std::shared_ptr<void> m_metadata;

    int64_t m_offset;
    int32_t m_partition;
    std::chrono::system_clock::time_point m_timestamp;

    int m_retries;
    FlagSet m_flags;
    int32_t m_sequence_number;
    int16_t m_producer_epoch;
    bool m_has_sequence;

    ProducerMessage();
    int ByteSize(int version) const;
    void Clear();
};
