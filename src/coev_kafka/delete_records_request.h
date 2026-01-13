#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"
#include "encoder_decoder.h"

struct DeleteRecordsRequestTopic : versionedDecoder, IEncoder
{

  std::unordered_map<int32_t, int64_t> m_partition_offsets;
  int encode(packetEncoder &pe);
  int decode(packetDecoder &pd, int16_t version);
};

struct DeleteRecordsRequest : protocol_body
{

  int16_t m_version;
  std::unordered_map<std::string, std::shared_ptr<DeleteRecordsRequestTopic>> m_topics;
  std::chrono::milliseconds m_timeout;

  DeleteRecordsRequest() = default;
  DeleteRecordsRequest(int16_t v) : m_version(v)
  {
  }
  ~DeleteRecordsRequest();
  void set_version(int16_t v);
  int encode(packetEncoder &pe);
  int decode(packetDecoder &pd, int16_t version);
  int16_t key() const;
  int16_t version() const;
  int16_t header_version() const;
  bool is_valid_version() const;
  KafkaVersion required_version() const;
};
