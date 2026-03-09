/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <coev/coev.h>
#include "consumer.h"
#include "fetch_request.h"
#include "consumer_message.h"

namespace coev::kafka
{
    struct PartitionConsumer : std::enable_shared_from_this<PartitionConsumer>
    {
        int64_t m_high_water_mark_offset = 0;
        std::shared_ptr<Consumer> m_consumer;
        std::shared_ptr<Config> m_conf;
        std::shared_ptr<BrokerConsumer> m_broker;
        co_channel<std::shared_ptr<ConsumerMessage>> m_messages;
        co_channel<std::shared_ptr<ConsumerError>> m_errors;
        co_channel<std::shared_ptr<FetchResponse>> m_feeder;

        int32_t m_leader_epoch = 0;
        int32_t m_preferred_read_replica = InvalidPreferredReplicaID;

        co_channel<bool> m_trigger;
        co_channel<bool> m_dying;
        std::string m_topic;
        int32_t m_partition;
        KError m_response_result;
        int32_t m_fetch_size;
        int64_t m_offset = 0;
        int32_t m_retries = 0;
        bool m_paused = false;

        PartitionConsumer() = default;
        void SendError(KError err);
        void SendError(int err);
        void AsyncClose();

        std::chrono::duration<double> ComputeBackoff();

        awaitable<int> Dispatcher();
        awaitable<int> PreferredBroker(std::shared_ptr<Broker> &broker, int32_t &epoch);
        awaitable<int> Dispatch();
        awaitable<int> ChooseStartingOffset(int64_t offset);
        awaitable<int> Interceptors(std::shared_ptr<ConsumerMessage> &msg);
        awaitable<int> Close();

        auto &Messages() { return m_messages; }

        int64_t HighWaterMarkOffset();

        awaitable<void> ResponseFeeder();
        int ParseMessages(std::shared_ptr<MessageSet> msg_set, std::vector<std::shared_ptr<ConsumerMessage>> &messages);
        int ParseRecords(std::shared_ptr<RecordBatch> &batch, std::vector<std::shared_ptr<ConsumerMessage>> &messages);
        int ParseResponse(std::shared_ptr<FetchResponse> response, std::vector<std::shared_ptr<ConsumerMessage>> &messages);

        void Pause();
        void Resume();
        void PauseAll();
        void ResumeAll();
        bool IsPaused();
    };
} // namespace coev::kafka
