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
#include "undefined.h"
#include "produce_set.h"
#include "transaction_manager.h"
#include "producer_message.h"
#include "producer_error.h"


struct BrokerProducer;
struct TopicProducer;

struct AsyncProducer : std::enable_shared_from_this<AsyncProducer>
{
    AsyncProducer() = default;
    AsyncProducer(std::shared_ptr<Client> client, std::shared_ptr<TransactionManager> txnmgr);
    coev::awaitable<int> close();
    void async_close();
    bool is_transactional();
    ProducerTxnStatusFlag txn_status();
    int begin_txn();
    coev::awaitable<int> commit_txn();
    coev::awaitable<int> abort_txn();
    int add_offsets_to_txn(const std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> &offsets, const std::string &group_id);
    int add_message_to_txn(std::shared_ptr<ConsumerMessage> msg, const std::string &metadata, const std::string &group_id);

    coev::awaitable<void> dispatcher();
    coev::awaitable<void> retry_handler();
    coev::awaitable<void> shutdown();
    coev::awaitable<void> bump_idempotent_producer_epoch();
    int maybe_transition_to_error_state(KError err);
    void return_error(std::shared_ptr<ProducerMessage> msg, KError err);
    void return_error(std::shared_ptr<ProducerMessage> msg, int err) { return return_error(msg, (KError)err); }
    void return_errors(const std::vector<std::shared_ptr<ProducerMessage>> &batch, KError err);
    void return_errors(const std::vector<std::shared_ptr<ProducerMessage>> &batch, int err) { return return_errors(batch, (KError)err); }
    void return_successes(const std::vector<std::shared_ptr<ProducerMessage>> &batch);
    void retry_message(std::shared_ptr<ProducerMessage> msg, KError err);
    void retry_messages(const std::vector<std::shared_ptr<ProducerMessage>> &batch, KError err);
    std::shared_ptr<BrokerProducer> get_broker_producer(std::shared_ptr<Broker> &broker);
    void unref_broker_producer(std::shared_ptr<Broker> broker, std::shared_ptr<BrokerProducer> bp);
    coev::awaitable<void> abandon_broker_connection(std::shared_ptr<Broker> broker);
    coev::awaitable<int> finish_transaction(bool commit);
    coev::awaitable<void> retry_batch(const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> pSet, KError kerr);

    std::shared_ptr<Client> m_client;
    std::shared_ptr<Config> m_conf;
    std::shared_ptr<TransactionManager> m_txnmgr;
    coev::co_waitgroup m_in_flight;

    std::map<int32_t, std::shared_ptr<BrokerProducer>> m_brokers;
    std::map<std::shared_ptr<BrokerProducer>, int> m_broker_refs;

    coev::co_channel<std::shared_ptr<ProducerMessage>> m_input;
    coev::co_channel<std::shared_ptr<ProducerMessage>> m_retries;
    coev::co_channel<std::shared_ptr<ProducerError>> m_errors;
    coev::co_channel<std::shared_ptr<ProducerMessage>> m_successes;

    std::map<std::string, std::shared_ptr<TopicProducer>> m_topic_producer;
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