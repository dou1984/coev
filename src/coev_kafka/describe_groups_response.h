#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "api_versions.h"
#include "version.h"
#include "errors.h"
#include "consumer_group_members.h"
#include "balance_strategy.h"
#include "describe_groups_response.h"
#include "protocol_body.h"

struct GroupMemberDescription : VEncoder, VDecoder
{

    mutable int16_t m_version;
    std::string m_member_id;
    std::string m_group_instance_id;
    std::string m_client_id;
    std::string m_client_host;
    std::string m_member_metadata;
    std::string m_member_assignment;

    int encode(packet_encoder &pe, int16_t version) const;
    int decode(packet_decoder &pd, int16_t version);
    std::shared_ptr<ConsumerGroupMemberAssignment> get_member_assignment();
    std::shared_ptr<ConsumerGroupMemberMetadata> get_member_metadata();
};

struct GroupDescription : VEncoder, VDecoder
{
    mutable int16_t m_version;
    KError m_code;
    std::string m_message;
    std::string m_group_id;
    std::string m_state;
    std::string m_protocol_type;
    std::string m_protocol;
    std::map<std::string, std::shared_ptr<GroupMemberDescription>> m_members;
    int32_t m_authorized_operations;

    int encode(packet_encoder &pe, int16_t version) const;
    int decode(packet_decoder &pd, int16_t version);
};

struct DescribeGroupsResponse : protocol_body, flexible_version
{
    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    std::vector<GroupDescription> m_groups;

    void set_version(int16_t v);
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool is_flexible() const;
    bool is_flexible_version(int16_t version) const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
};
