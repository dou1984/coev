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
#include "sleep_for.h"

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
using TopicPartitionOffsets = std::unordered_map<TopicPartition, PartitionOffsetMetadata, TopicPartition::Hash>;

struct TransactionManager
{
    struct Result
    {
        bool Retry = false;
        int Error = 0;
    };
    TransactionManager() = default;
    TransactionManager(std::shared_ptr<Config> conf, std::shared_ptr<Client> client);
    ~TransactionManager() = default;

    ProducerTxnStatusFlag current_txn_status() const;
    void get_and_increment_sequence_number(const std::string &topic, int32_t partition, int32_t &sequence, int16_t &epoch);
    void bump_epoch();
    std::pair<int64_t, int16_t> get_producer_id();
    std::chrono::milliseconds compute_backoff(int attempts_remaining) const;
    bool is_transactional() const;
    int add_offsets_to_txn(const std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> &offsetsToAdd, const std::string &groupId);
    coev::awaitable<int> publish_offsets_to_txn(const TopicPartitionOffsets &offsets, const std::string &groupId, TopicPartitionOffsets &outOffsets);
    coev::awaitable<Result> publish_offsets_to_txn_add_offset(const std::string &groupId);
    coev::awaitable<Result> publish_offsets_to_txn_commit(const TopicPartitionOffsets &offsets, const std::string &groupId, TopicPartitionOffsets &outOffsets);
    coev::awaitable<int> init_producer_id(int64_t &producerID, int16_t &producerEpoch);
    int abortable_error_if_possible(int err);
    int complete_transaction();

    coev::awaitable<int> end_txn(bool commit);
    coev::awaitable<int> finish_transaction(bool commit);
    coev::awaitable<int> publish_txn_partitions();
    coev::awaitable<int> initialize_transactions();
    void maybe_add_partition_to_current_txn(const std::string &topic, int32_t partition);
    bool is_transition_valid(ProducerTxnStatusFlag target) const;
    int transition_to(ProducerTxnStatusFlag target, int err);

    template <class Func, class... Args>
    coev::awaitable<int> Retry(int attempts_remaining, const Func &run, Args &&...args)
    {
        while (attempts_remaining >= 0)
        {
            auto r = co_await run(std::forward<Args>(args)...);
            if (!r.Retry)
            {
                co_return r.Error;
            }
            auto backoff = compute_backoff(attempts_remaining);
            LOG_CORE("retrying after %lldms %d attempts remaining %d",
                     static_cast<long long>(backoff.count() / 1000000), attempts_remaining, static_cast<int>(r.Error));

            co_await sleep_for(backoff);
            attempts_remaining--;
        }
        co_return 0;
    }

    ProducerTxnStatusFlag m_status = ProducerTxnFlagUninitialized;
    int64_t m_producer_id = noProducerID;
    int16_t m_producer_epoch = noProducerEpoch;
    std::map<std::string, int32_t> m_sequence_numbers;
    std::string m_transactional_id;
    std::chrono::milliseconds m_transaction_timeout;
    std::shared_ptr<Client> m_client;
    bool m_coordinator_supports_bumping_epoch = false;
    bool m_epoch_bump_required = false;
    int m_last_error = 0;
    TopicPartitionSet m_pending_partitions_in_current_txn;
    TopicPartitionSet m_partitions_in_current_txn;
    std::map<std::string, TopicPartitionOffsets> m_offsets_in_current_txn;
};
