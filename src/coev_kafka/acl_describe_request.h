#pragma once

#include <cstdint>
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "api_versions.h"
#include "protocol_body.h"
#include "acl_filter.h"

struct DescribeAclsRequest : protocol_body
{
    int m_version;
    AclFilter m_filter;

    DescribeAclsRequest() = default;
    DescribeAclsRequest(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
};
