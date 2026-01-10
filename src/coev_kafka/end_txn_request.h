#pragma once

#include <string>
#include <cstdint>
#include "packet_decoder.h"
#include "packet_encoder.h"
#include "version.h"
#include "api_versions.h"
#include "protocol_body.h"

struct EndTxnRequest : protocol_body
{

	int16_t m_version;
	std::string m_transactional_id;
	int64_t m_producer_id;
	int16_t m_producer_epoch;
	bool m_transaction_result;
	EndTxnRequest() = default;
	EndTxnRequest(int16_t v) : m_version(v)
	{
	}
	void set_version(int16_t v);
	int encode(PEncoder &pe);
	int decode(PDecoder &pd, int16_t version);
	int16_t key() const;
	int16_t version() const;
	int16_t headerVersion() const;
	bool is_valid_version() const;
	KafkaVersion required_version() const;
};
