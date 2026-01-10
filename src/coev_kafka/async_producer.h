#pragma once

#include <memory>
#include <string>
#include <map>
#include <mutex>
#include <atomic>
#include <coev/coev.h>
#include "client.h"
#include "config.h"
#include "broker.h"
#include "metrics.h"
#include "undefined.h"
#include "produce_set.h"
#include "transaction_manager.h"
#include "producer_message.h"
#include "producer_error.h"

struct BrokerProducerResponse;
struct BrokerProducer;

struct AsyncProducer : std::enable_shared_from_this<AsyncProducer>
{
    AsyncProducer() = default;
    coev::awaitable<int> Close();
    void AsyncClose();
    bool IsTransactional();
    ProducerTxnStatusFlag TxnStatus();
    int BeginTxn();
    coev::awaitable<int> CommitTxn();
    coev::awaitable<int> AbortTxn();
    int AddOffsetsToTxn(const std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> &offsets, const std::string &group_id);
    int AddMessageToTxn(std::shared_ptr<ConsumerMessage> msg, const std::string &metadata, const std::string &group_id);

    AsyncProducer(std::shared_ptr<Client> client, std::shared_ptr<TransactionManager> txnmgr);

    coev::awaitable<void> dispatcher();
    coev::awaitable<void> retryHandler();
    coev::awaitable<void> shutdown();
    coev::awaitable<void> bumpIdempotentProducerEpoch();
    int maybeTransitionToErrorState(KError err);
    void returnError(std::shared_ptr<ProducerMessage> msg, KError err);
    void returnError(std::shared_ptr<ProducerMessage> msg, int err) { return returnError(msg, (KError)err); }
    void returnErrors(const std::vector<std::shared_ptr<ProducerMessage>> &batch, KError err);
    void returnErrors(const std::vector<std::shared_ptr<ProducerMessage>> &batch, int err) { return returnErrors(batch, (KError)err); }
    void returnSuccesses(const std::vector<std::shared_ptr<ProducerMessage>> &batch);
    void retryMessage(std::shared_ptr<ProducerMessage> msg, KError err);
    void retryMessages(const std::vector<std::shared_ptr<ProducerMessage>> &batch, KError err);
    std::shared_ptr<BrokerProducer> getBrokerProducer(std::shared_ptr<Broker> &broker);
    void unrefBrokerProducer(std::shared_ptr<Broker> broker, std::shared_ptr<BrokerProducer> bp);
    coev::awaitable<void> abandonBrokerConnection(std::shared_ptr<Broker> broker);
    coev::awaitable<int> finishTransaction(bool commit);
    coev::awaitable<void> retryBatch(const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> pSet, KError kerr);

    std::shared_ptr<Client> m_client;
    std::shared_ptr<Config> m_conf;
    std::shared_ptr<TransactionManager> m_txnmgr;
    std::atomic<int> m_in_flight;

    std::map<std::shared_ptr<Broker>, std::shared_ptr<BrokerProducer>> m_brokers;
    std::map<std::shared_ptr<BrokerProducer>, int> m_broker_refs;
    std::mutex m_broker_lock;
    std::mutex m_tx_lock;

    std::shared_ptr<metrics::Registry> m_metrics_registry;

    coev::co_channel<std::shared_ptr<ProducerError>> m_errors;
    coev::co_channel<std::shared_ptr<ProducerMessage>> m_input;
    coev::co_channel<std::shared_ptr<ProducerMessage>> m_successes;
    coev::co_channel<std::shared_ptr<ProducerMessage>> m_retries;
    coev::co_task m_task;
};

struct PartitionRetryState
{
    std::vector<std::shared_ptr<ProducerMessage>> m_buf;
    bool m_expect_chaser;
};

coev::awaitable<int> NewAsyncProducer(const std::vector<std::string> &addrs, std::shared_ptr<Config> conf, std::shared_ptr<AsyncProducer> &producer);
coev::awaitable<int> NewAsyncProducerFromClient(std::shared_ptr<Client> client, std::shared_ptr<AsyncProducer> &producer);
coev::awaitable<int> NewAsyncProducer(std::shared_ptr<Client> client, std::shared_ptr<AsyncProducer> &producer);