#pragma once

#include <string>
#include <map>
#include <cstdint>
#include <chrono>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"
#include "throttle_support.h"

struct DeleteGroupsResponse : protocol_body, flexible_version, throttle_support
{
	int16_t m_version;
	std::chrono::milliseconds m_throttle_time;
	std::map<std::string, KError> m_group_error_codes;
	DeleteGroupsResponse() = default;
	DeleteGroupsResponse(int16_t v) : m_version(v)
	{
	}
	void set_version(int16_t v);
	int encode(packet_encoder &pe) const;
	int decode(packet_decoder &pd, int16_t version);
	int16_t key() const;
	int16_t version() const;
	int16_t header_version() const;
	bool is_flexible() const;
	bool is_flexible_version(int16_t version) const;
	bool is_valid_version() const;
	KafkaVersion required_version() const;
	std::chrono::milliseconds throttle_time() const;
};
