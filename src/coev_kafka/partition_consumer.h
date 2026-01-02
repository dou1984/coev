#pragma once
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <coev.h>
#include "consumer.h"
#include "fetch_request.h"
#include "consumer_message.h"

struct PartitionConsumer : std::enable_shared_from_this<PartitionConsumer>
{

    std::atomic<int64_t> highWaterMarkOffset{0};
    std::shared_ptr<Consumer> consumer;
    std::shared_ptr<Config> conf;
    std::shared_ptr<BrokerConsumer> broker;
    coev::co_channel<std::shared_ptr<ConsumerMessage>> messages;
    coev::co_channel<std::shared_ptr<ConsumerError>> errors;
    coev::co_channel<std::shared_ptr<FetchResponse>> feeder;

    int32_t leaderEpoch = 0;
    int32_t preferredReadReplica = invalidPreferredReplicaID;

    coev::co_channel<bool> trigger;
    coev::co_channel<bool> dying;
    std::string topic;
    int32_t partition;
    KError responseResult;
    int32_t fetchSize;
    int64_t offset;
    std::atomic<int32_t> retries{0};
    std::atomic<bool> paused{false};

    PartitionConsumer() = default;
    void SendError(KError err);
    void SendError(int err);
    void AsyncClose();

    std::chrono::duration<double> ComputeBackoff();

    coev::awaitable<int> Dispatcher();
    coev::awaitable<int> preferredBroker(std::shared_ptr<Broker> &broker, int32_t &epoch);
    coev::awaitable<int> Dispatch();
    coev::awaitable<int> ChooseStartingOffset(int64_t offset);
    coev::awaitable<int> Interceptors(std::shared_ptr<ConsumerMessage> &msg);
    coev::awaitable<int> Close();

    int64_t HighWaterMarkOffset();

    coev::awaitable<void> ResponseFeeder();
    int ParseMessages(std::shared_ptr<MessageSet> msgSet, std::vector<std::shared_ptr<ConsumerMessage>> &messages);
    int ParseRecords(std::shared_ptr<RecordBatch> batch, std::vector<std::shared_ptr<ConsumerMessage>> &messages);
    int ParseResponse(std::shared_ptr<FetchResponse> response, std::vector<std::shared_ptr<ConsumerMessage>> &messages);

    void Pause();
    void Resume();
    void PauseAll();
    void ResumeAll();
    bool IsPaused();
};
