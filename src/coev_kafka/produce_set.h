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
    std::vector<std::shared_ptr<ProducerMessage>> m_messages;
    std::shared_ptr<Records> m_records;
    int m_buffer_bytes = 0;
};

struct ProduceSet
{

    std::shared_ptr<AsyncProducer> m_parent;
    std::unordered_map<std::string, std::map<int32_t, std::shared_ptr<PartitionSet>>> m_messages;
    int64_t m_producer_id = 0;
    int16_t m_producer_epoch = 0;
    int m_buffer_bytes = 0;
    int m_buffer_count = 0;

    ProduceSet(std::shared_ptr<AsyncProducer> parent);
    int add(std::shared_ptr<ProducerMessage> msg);
    std::shared_ptr<ProduceRequest> build_request();

    std::vector<std::shared_ptr<ProducerMessage>> drop_partition(const std::string &topic, int32_t partition);
    bool would_overflow(std::shared_ptr<ProducerMessage> msg);
    bool ready_to_flush();
    bool empty() const;

    template <class F, class... Args>
    void each_partition(const F &_f, Args &&...args)
    {
        for (const auto &[topic, partitions] : m_messages)
        {
            for (const auto &[partition, pset] : partitions)
            {
                _f(topic, partition, pset, std::forward<Args>(args)...);
            }
        }
    }
};
