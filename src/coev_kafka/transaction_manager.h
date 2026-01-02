#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include "broker.h"
#include "version.h"
#include "topic_partition.h"

enum ProducerTxnStatusFlag : uint16_t
{
    ProducerTxnFlagUninitialized = 1 << 0,
    ProducerTxnFlagInitializing = 1 << 1,
    ProducerTxnFlagReady = 1 << 2,
    ProducerTxnFlagInTransaction = 1 << 3,
    ProducerTxnFlagEndTransaction = 1 << 4,
    ProducerTxnFlagInError = 1 << 5,
    ProducerTxnFlagCommittingTransaction = 1 << 6,
    ProducerTxnFlagAbortingTransaction = 1 << 7,
    ProducerTxnFlagAbortableError = 1 << 8,
    ProducerTxnFlagFatalError = 1 << 9,
};

inline constexpr int64_t noProducerID = -1;
inline constexpr int16_t noProducerEpoch = -1;
inline constexpr std::chrono::milliseconds addPartitionsRetryBackoff(20);

using TopicPartitionSet = std::unordered_map<TopicPartition, std::monostate, TopicPartition::Hash>;
using TopicPartitionOffsets = std::unordered_map<TopicPartition, std::shared_ptr<PartitionOffsetMetadata>, TopicPartition::Hash>;

struct TransactionManager
{
    struct Result
    {
        bool Retry;
        int Error;
    };
    TransactionManager() = default;
    ~TransactionManager() = default;

    ProducerTxnStatusFlag CurrentTxnStatus() const;
    void GetAndIncrementSequenceNumber(const std::string &topic, int32_t partition, int32_t &sequence, int16_t &epoch);
    void BumpEpoch();
    std::pair<int64_t, int16_t> GetProducerID();
    std::chrono::milliseconds ComputeBackoff(int attemptsRemaining) const;
    bool IsTransactional() const;
    int AddOffsetsToTxn(const std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> &offsetsToAdd, const std::string &groupId);
    coev::awaitable<int> Retry(int attemptsRemaining, const std::function<coev::awaitable<Result>()> &run);
    coev::awaitable<int> PublishOffsetsToTxn(const TopicPartitionOffsets &offsets, const std::string &groupId, TopicPartitionOffsets &outOffsets);
    coev::awaitable<Result> PublishOffsetsToTxnAddOffset(const std::string &groupId);
    coev::awaitable<Result> PublishOffsetsToTxnCommit(const TopicPartitionOffsets &offsets, const std::string &groupId, TopicPartitionOffsets &outOffsets);
    coev::awaitable<int> InitProducerId(int64_t &producerID, int16_t &producerEpoch);
    int AbortableErrorIfPossible(int err);
    int CompleteTransaction();

    coev::awaitable<int> EndTxn(bool commit);
    coev::awaitable<int> FinishTransaction(bool commit);
    void MaybeAddPartitionToCurrentTxn(const std::string &topic, int32_t partition);
    coev::awaitable<int> PublishTxnPartitions();
    coev::awaitable<int> InitializeTransactions();
    bool IsTransitionValid(ProducerTxnStatusFlag target) const;
    int TransitionTo(ProducerTxnStatusFlag target, int err);

    mutable std::shared_mutex statusLock_;
    ProducerTxnStatusFlag status_;
    mutable std::mutex mutex_;
    int64_t producerID_;
    int16_t producerEpoch_;
    std::map<std::string, int32_t> sequenceNumbers_;
    std::string transactionalID_;
    std::chrono::milliseconds transactionTimeout_;
    std::shared_ptr<Client> client_;
    bool coordinatorSupportsBumpingEpoch_;
    bool epochBumpRequired_;
    int lastError_;
    mutable std::mutex partitionInTxnLock_;
    TopicPartitionSet pendingPartitionsInCurrentTxn_;
    TopicPartitionSet partitionsInCurrentTxn_;
    std::map<std::string, TopicPartitionOffsets> offsetsInCurrentTxn_;
};

coev::awaitable<int> NewTransactionManager(std::shared_ptr<Config> conf, std::shared_ptr<Client> client, std::shared_ptr<TransactionManager> &txnmgr);