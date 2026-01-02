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
struct DeleteOffsetsRequest : protocolBody
{
	int16_t Version;
	std::string Group;
	std::unordered_map<std::string, std::vector<int32_t>> partitions;
	DeleteOffsetsRequest() = default;
	DeleteOffsetsRequest(int16_t v) : Version(v)
	{
	}
	void setVersion(int16_t v);
	int encode(PEncoder &pe);
	int decode(PDecoder &pd, int16_t version);
	int16_t key() const;
	int16_t version() const;
	int16_t headerVersion() const;
	bool isValidVersion() const;
	KafkaVersion requiredVersion() const;
	void AddPartition(const std::string &topic, int32_t partitionID);
};
