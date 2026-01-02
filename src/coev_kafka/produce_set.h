#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <functional>

#include "producer_message.h"
#include "records.h"
#include "record_batch.h"
#include "message_set.h"
#include "version.h"
#include "produce_request.h"

inline constexpr int recordBatchOverhead = 49;
struct AsyncProducer;

struct PartitionSet
{
    std::vector<std::shared_ptr<ProducerMessage>> msgs;
    std::shared_ptr<Records> recordsToSend;
    int bufferBytes = 0;
};

struct ProduceSet
{

    std::shared_ptr<AsyncProducer> Parent;
    std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<PartitionSet>>> msgs;
    int64_t producerID = 0;
    int16_t producerEpoch = 0;
    int bufferBytes = 0;
    int bufferCount = 0;

    ProduceSet(std::shared_ptr<AsyncProducer> parent);
    int Add(std::shared_ptr<ProducerMessage> msg);
    std::shared_ptr<ProduceRequest> BuildRequest();
    void EachPartition(std::function<void(const std::string &, int32_t, std::shared_ptr<PartitionSet>)> cb);
    std::vector<std::shared_ptr<ProducerMessage>> DropPartition(const std::string &topic, int32_t partition);
    bool WouldOverflow(std::shared_ptr<ProducerMessage> msg);
    bool ReadyToFlush();
    bool Empty() const;
};
