#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "api_versions.h"
#include "protocol_body.h"
#include "consumer_group_members.h"

struct GroupProtocol : IDecoder, IEncoder
{

    std::string m_name;
    std::string m_metadata;

    GroupProtocol() = default;
    GroupProtocol(const std::string &name, const std::string &metadata);
    int decode(packet_decoder &pd);
    int encode(packet_encoder &pe) const;
};

struct JoinGroupRequest : protocol_body, flexible_version
{

    int16_t m_version = 0;
    std::string m_group_id;
    int32_t m_session_timeout;
    int32_t m_rebalance_timeout;
    std::string m_member_id;
    std::string m_group_instance_id;
    std::string m_protocol_type;
    std::map<std::string, std::string> m_group_protocols;
    std::vector<GroupProtocol> m_ordered_group_protocols;
    JoinGroupRequest() = default;
    JoinGroupRequest(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool is_flexible() const;
    bool is_flexible_version(int16_t ver) const;
    KafkaVersion required_version() const;
    void add_group_protocol(const std::string &name, const std::string &metadata);
    int add_group_protocol_metadata(const std::string &name, const std::shared_ptr<ConsumerGroupMemberMetadata> &metadata);
};
