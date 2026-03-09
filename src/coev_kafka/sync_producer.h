/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
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

namespace coev::kafka
{
    struct SyncProducer
    {

        std::shared_ptr<AsyncProducer> m_producer;
        std::atomic<bool> closed{false};
        coev::co_task m_task;

        SyncProducer(std::shared_ptr<AsyncProducer> producer);
        ~SyncProducer();

        awaitable<int> send_message(std::shared_ptr<ProducerMessage> msg, int32_t &, int64_t &);
        awaitable<int> send_messages(const std::vector<std::shared_ptr<ProducerMessage>> &msgs);
        int close();
        ProducerTxnStatusFlag txn_status();
        bool is_transactional();
        int begin_txn();
        awaitable<int> commit_txn();
        awaitable<int> abort_txn();
        awaitable<void> handle_replies();
        int add_offsets_to_txn(const std::map<std::string, std::vector<PartitionOffsetMetadata>> &offsets, const std::string &group_id);
        int add_message_to_txn(std::shared_ptr<ConsumerMessage> msg, const std::string &metadata, const std::string &group_id);
    };

    awaitable<int> NewSyncProducer(const std::vector<std::string> &addrs, std::shared_ptr<Config> &config, std::shared_ptr<SyncProducer> &);
    awaitable<int> NewSyncProducerFromClient(std::shared_ptr<Client> client, std::shared_ptr<SyncProducer> &);

}