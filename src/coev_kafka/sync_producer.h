#pragma once

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <coev.h>
#include "version.h"
#include "client.h"
#include "consumer.h"
#include "async_producer.h"
#include "partition_offset_metadata.h"
#include "transaction_manager.h"

struct ISyncProducer
{

    virtual ~ISyncProducer() = default;

    virtual coev::awaitable<int> SendMessage(std::shared_ptr<ProducerMessage> msg, int32_t &, int64_t &) = 0;
    virtual coev::awaitable<int> SendMessages(const std::vector<std::shared_ptr<ProducerMessage>> &msgs) = 0;
    virtual int Close() = 0;
    virtual ProducerTxnStatusFlag TxnStatus() = 0;
    virtual bool IsTransactional() = 0;
    virtual int BeginTxn() = 0;
    virtual coev::awaitable<int> CommitTxn() = 0;
    virtual coev::awaitable<int> AbortTxn() = 0;
    virtual int AddOffsetsToTxn(const std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> &offsets, const std::string &group_id) = 0;
    virtual int AddMessageToTxn(std::shared_ptr<ConsumerMessage> msg, const std::string &metadata, const std::string &group_id) = 0;
};

coev::awaitable<int> NewSyncProducer(const std::vector<std::string> &addrs, std::shared_ptr<Config> &config, std::shared_ptr<ISyncProducer> &);

coev::awaitable<int> NewSyncProducerFromClient(std::shared_ptr<Client> client, std::shared_ptr<ISyncProducer> &);
