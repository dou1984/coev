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

struct AsyncProducer;

struct PartitionSet
{
    std::vector<std::shared_ptr<ProducerMessage>> m_msgs;
    std::shared_ptr<Records> m_records_to_send;
    int m_buffer_bytes = 0;
};

struct ProduceSet
{

    std::shared_ptr<AsyncProducer> m_parent;
    std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<PartitionSet>>> m_messages;
    int64_t m_producer_id = 0;
    int16_t m_producer_epoch = 0;
    int m_buffer_bytes = 0;
    int m_buffer_count = 0;

    ProduceSet(std::shared_ptr<AsyncProducer> parent);
    int add(std::shared_ptr<ProducerMessage> msg);
    std::shared_ptr<ProduceRequest> build_request();
    void each_partition(std::function<void(const std::string &, int32_t, std::shared_ptr<PartitionSet>)> cb);
    std::vector<std::shared_ptr<ProducerMessage>> drop_partition(const std::string &topic, int32_t partition);
    bool would_overflow(std::shared_ptr<ProducerMessage> msg);
    bool ready_to_flush();
    bool empty() const;
};
