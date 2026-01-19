#pragma once

#include <string>
#include <map>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"
struct DeleteGroupsResponse : protocol_body, flexible_version
{
	int16_t m_version;
	std::chrono::milliseconds m_throttle_time;
	std::map<std::string, KError> m_group_error_codes;
	DeleteGroupsResponse() = default;
	DeleteGroupsResponse(int16_t v) : m_version(v)
	{
	}
	void set_version(int16_t v);
	int encode(packetEncoder &pe);
	int decode(packetDecoder &pd, int16_t version);
	int16_t key() const;
	int16_t version() const;
	int16_t header_version() const;
	bool is_flexible() const;
	bool is_flexible_version(int16_t version) const;
	bool is_valid_version() const;
	KafkaVersion required_version() const;
	std::chrono::milliseconds throttle_time() const;
};
