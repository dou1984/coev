#pragma once

#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <cstdint>
#include "acl_bindings.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "version.h"
#include "api_versions.h"
#include "protocol_body.h"

struct MatchingAcl : VEncoder, VDecoder
{
    KError m_err = ErrNoError;
    std::string m_err_msg;
    Resource m_resource;
    Acl m_acl;

    int encode(packet_encoder &pe, int16_t version) const;
    int decode(packet_decoder &pd, int16_t version);
};

struct FilterResponse : VEncoder, VDecoder
{
    KError m_err = ErrNoError;
    std::string m_err_msg;
    std::vector<MatchingAcl> m_matching_acls;

    int encode(packet_encoder &pe, int16_t version) const;
    int decode(packet_decoder &pd, int16_t version);
};

struct DeleteAclsResponse : protocol_body
{
    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    std::vector<FilterResponse> m_filter_responses;

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
