#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <chrono>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "utils.h"
#include "version.h"
#include "protocol_body.h"
#include "partition_error.h"

struct TxnOffsetCommitResponse : protocol_body
{

    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    std::map<std::string, std::vector<std::shared_ptr<PartitionError>>> m_topics;

    void set_version(int16_t v);
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
};
