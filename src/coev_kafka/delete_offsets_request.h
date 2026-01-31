#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

#include "packet_decoder.h"
#include "packet_encoder.h"
#include "version.h"
#include "api_versions.h"
#include "protocol_body.h"
struct DeleteOffsetsRequest : protocol_body
{
	int16_t m_version;
	std::string m_group;
	std::unordered_map<std::string, std::vector<int32_t>> m_partitions;
	DeleteOffsetsRequest() = default;
	DeleteOffsetsRequest(int16_t v) : m_version(v)
	{
	}
	void set_version(int16_t v);
	int encode(packetEncoder &pe) const;
    int decode(packetDecoder &pd, int16_t version);
	int16_t key() const;
	int16_t version() const;
	int16_t header_version() const;
	bool is_valid_version() const;
	KafkaVersion required_version() const;
	void AddPartition(const std::string &topic, int32_t partitionID);
};
