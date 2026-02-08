#pragma once

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <coev/coev.h>
#include "version.h"
#include "client.h"
#include "consumer.h"
#include "async_producer.h"
#include "partition_offset_metadata.h"
#include "transaction_manager.h"



struct SyncProducer
{

    std::shared_ptr<AsyncProducer> m_producer;
    std::atomic<bool> closed{false};
    coev::co_task m_task;

    coev::awaitable<void> handleSuccesses();
    coev::awaitable<void> handleErrors();

    SyncProducer(std::shared_ptr<AsyncProducer> p);
    ~SyncProducer();

    coev::awaitable<int> SendMessage(std::shared_ptr<ProducerMessage> msg, int32_t &, int64_t &);
    coev::awaitable<int> SendMessages(const std::vector<std::shared_ptr<ProducerMessage>> &msgs);
    int Close();
    ProducerTxnStatusFlag TxnStatus();
    bool IsTransactional();
    int BeginTxn();
    coev::awaitable<int> CommitTxn();
    coev::awaitable<int> AbortTxn();
    int add_offsets_to_txn(const std::map<std::string, std::vector<PartitionOffsetMetadata>> &offsets, const std::string &group_id);
    int add_message_to_txn(std::shared_ptr<ConsumerMessage> msg, const std::string &metadata, const std::string &group_id);
};

coev::awaitable<int> NewSyncProducer(const std::vector<std::string> &addrs, std::shared_ptr<Config> &config, std::shared_ptr<SyncProducer> &);

coev::awaitable<int> NewSyncProducerFromClient(std::shared_ptr<Client> client, std::shared_ptr<SyncProducer> &);
