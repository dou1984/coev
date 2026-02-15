#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>

#include "packet_decoder.h"
#include "packet_encoder.h"
#include "protocol_body.h"
struct OwnedPartition;
struct ConsumerGroupMemberMetadata : IEncoder, IDecoder
{
    int16_t m_version = 0;
    int32_t m_generation_id = 0;
    std::string m_user_data;
    std::string m_rack_id;
    std::vector<std::string> m_topics;
    std::vector<OwnedPartition> m_owned_partitions;

    ConsumerGroupMemberMetadata() = default;
    ConsumerGroupMemberMetadata(int16_t v) : m_version(v) {}

    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd);
};

struct OwnedPartition : IEncoder, IDecoder
{
    std::string topic;
    std::vector<int32_t> partitions;

    OwnedPartition() = default;
    OwnedPartition(const std::string &t, const std::vector<int32_t> &p) : topic(t), partitions(p) {}

    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd);
};

struct ConsumerGroupMemberAssignment : IEncoder, IDecoder
{
    int16_t m_version = 0;
    std::map<std::string, std::vector<int32_t>> m_topics;
    std::string m_userdata;

    ConsumerGroupMemberAssignment() = default;
    ConsumerGroupMemberAssignment(int16_t v) : m_version(v) {}

    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd);
};
