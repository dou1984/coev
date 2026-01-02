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

struct DeleteRecordsRequestTopic : VDecoder, IEncoder
{

  std::unordered_map<int32_t, int64_t> PartitionOffsets;
  int encode(PEncoder &pe);
  int decode(PDecoder &pd, int16_t version);
};

struct DeleteRecordsRequest : protocolBody
{

  int16_t Version;
  std::unordered_map<std::string, std::shared_ptr<DeleteRecordsRequestTopic>> Topics;
  std::chrono::milliseconds Timeout;

  DeleteRecordsRequest() = default;
  DeleteRecordsRequest(int16_t v) : Version(v)
  {
  }
  ~DeleteRecordsRequest();
  void setVersion(int16_t v);
  int encode(PEncoder &pe);
  int decode(PDecoder &pd, int16_t version);
  int16_t key() const;
  int16_t version() const;
  int16_t headerVersion() const;
  bool isValidVersion() const;
  KafkaVersion requiredVersion() const;
};
