#include "transaction_manager.h"
#include "sleep_for.h"
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <chrono>
#include <functional>
#include <tuple>

static std::unordered_map<ProducerTxnStatusFlag, std::vector<ProducerTxnStatusFlag>> producerTxnTransitions = {
    {ProducerTxnFlagUninitialized, {ProducerTxnFlagReady, ProducerTxnFlagInError}},
    {ProducerTxnFlagInitializing, {ProducerTxnFlagInitializing, ProducerTxnFlagReady, ProducerTxnFlagInError}},
    {ProducerTxnFlagReady, {ProducerTxnFlagInTransaction}},
    {ProducerTxnFlagInTransaction, {ProducerTxnFlagEndTransaction, ProducerTxnFlagInError}},
    {ProducerTxnFlagEndTransaction, {ProducerTxnFlagInitializing, ProducerTxnFlagReady, ProducerTxnFlagInError}},
    {ProducerTxnFlagAbortableError, {ProducerTxnFlagAbortingTransaction, ProducerTxnFlagInError}},
    {ProducerTxnFlagFatalError, {ProducerTxnFlagFatalError}}};

std::string to_string(ProducerTxnStatusFlag s)
{
    std::vector<std::string> status;
    if (s & ProducerTxnFlagUninitialized)
        status.push_back("ProducerTxnStateUninitialized");
    if (s & ProducerTxnFlagInitializing)
        status.push_back("ProducerTxnStateInitializing");
    if (s & ProducerTxnFlagReady)
        status.push_back("ProducerTxnStateReady");
    if (s & ProducerTxnFlagInTransaction)
        status.push_back("ProducerTxnStateInTransaction");
    if (s & ProducerTxnFlagEndTransaction)
        status.push_back("ProducerTxnStateEndTransaction");
    if (s & ProducerTxnFlagInError)
        status.push_back("ProducerTxnStateInError");
    if (s & ProducerTxnFlagCommittingTransaction)
        status.push_back("ProducerTxnStateCommittingTransaction");
    if (s & ProducerTxnFlagAbortingTransaction)
        status.push_back("ProducerTxnStateAbortingTransaction");
    if (s & ProducerTxnFlagAbortableError)
        status.push_back("ProducerTxnStateAbortableError");
    if (s & ProducerTxnFlagFatalError)
        status.push_back("ProducerTxnStateFatalError");
    std::ostringstream oss;
    for (size_t i = 0; i < status.size(); ++i)
    {
        if (i > 0)
            oss << "|";
        oss << status[i];
    }
    return oss.str();
}

std::map<std::string, std::vector<int32_t>> MapToRequest(const TopicPartitionSet &s)
{
    std::map<std::string, std::vector<int32_t>> result;
    for (auto &kv : s)
    {
        result[kv.first.m_topic].push_back(kv.first.m_partition);
    }
    return result;
}

std::map<std::string, std::vector<PartitionOffsetMetadata>> MapToRequest(const TopicPartitionOffsets &_offsets)
{
    std::map<std::string, std::vector<PartitionOffsetMetadata>> result;
    for (auto &[tp, pfm] : _offsets)
    {
        result[tp.m_topic].push_back(pfm);
    }
    return result;
}

bool TransactionManager::is_transition_valid(ProducerTxnStatusFlag target) const
{
    for (auto &[status, allowedTransitions] : producerTxnTransitions)
    {
        if ((m_status & status) != 0)
        {
            for (ProducerTxnStatusFlag allowed : allowedTransitions)
            {
                if ((target & allowed) != 0)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

ProducerTxnStatusFlag TransactionManager::current_txn_status() const
{
    return m_status;
}

int TransactionManager::transition_to(ProducerTxnStatusFlag target, int err)
{
    if (!is_transition_valid(target))
    {
        return ErrTransitionNotAllowed;
    }
    if ((target & ProducerTxnFlagInError) != 0)
    {
        if (err == 0)
        {
            return ErrCannotTransitionNilError;
        }
        m_last_error = err;
    }
    else
    {
        m_last_error = 0;
    }
    m_status = target;
    return err;
}

void TransactionManager::get_and_increment_sequence_number(const std::string &topic, int32_t partition, int32_t &sequence, int16_t &epoch)
{
    std::string key = topic + "-" + std::to_string(partition);
    sequence = m_sequence_numbers[key];
    m_sequence_numbers[key] = sequence + 1;
    epoch = m_producer_epoch;
}

void TransactionManager::bump_epoch()
{
    m_producer_epoch++;
    for (auto &kv : m_sequence_numbers)
    {
        kv.second = 0;
    }
}

std::pair<int64_t, int16_t> TransactionManager::get_producer_id()
{
    return {m_producer_id, m_producer_epoch};
}

std::chrono::milliseconds TransactionManager::compute_backoff(int attempts_remaining) const
{
    auto config = m_client->GetConfig();
    if (config->Producer.Transaction.Retry.BackoffFunc)
    {
        int maxRetries = config->Producer.Transaction.Retry.Max;
        int retries = maxRetries - attempts_remaining;
        return config->Producer.Transaction.Retry.BackoffFunc(retries, maxRetries);
    }
    return config->Producer.Transaction.Retry.Backoff;
}

bool TransactionManager::is_transactional() const
{
    return !m_transactional_id.empty();
}

int TransactionManager::add_offsets_to_txn(const std::map<std::string, std::vector<PartitionOffsetMetadata>> &_offsets, const std::string &groupId)
{
    if ((current_txn_status() & ProducerTxnFlagInTransaction) == 0)
    {
        return ErrTransactionNotReady;
    }
    if ((current_txn_status() & ProducerTxnFlagFatalError) != 0)
    {
        return m_last_error;
    }
    if (m_offsets_in_current_txn.find(groupId) == m_offsets_in_current_txn.end())
    {
        m_offsets_in_current_txn[groupId] = TopicPartitionOffsets{};
    }
    for (auto &[topic, topic_offsets] : _offsets)
    {
        for (auto &offset : topic_offsets)
        {
            TopicPartition tp(topic, offset.m_partition);
            m_offsets_in_current_txn[groupId][tp] = offset;
        }
    }
    return 0;
}

coev::awaitable<TransactionManager::Result> TransactionManager::publish_offsets_to_txn_add_offset(const std::string &groupId)
{
    std::shared_ptr<Broker> coordinator;
    int err = co_await m_client->TransactionCoordinator(m_transactional_id, coordinator);
    if (err != ErrNoError)
    {
        co_return {true, err};
    }

    auto request = std::make_shared<AddOffsetsToTxnRequest>();
    request->m_transactional_id = m_transactional_id;
    request->m_producer_epoch = m_producer_epoch;
    request->m_producer_id = m_producer_id;
    request->m_group_id = groupId;

    if (m_client->GetConfig()->Version.IsAtLeast(V2_7_0_0))
    {
        request->m_version = 2;
    }
    else if (m_client->GetConfig()->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 1;
    }

    ResponsePromise<AddOffsetsToTxnResponse> response;
    err = co_await coordinator->AddOffsetsToTxn(request, response);
    if (err != ErrNoError)
    {
        coordinator->Close();
        err = co_await m_client->RefreshTransactionCoordinator(m_transactional_id);
        if (err != ErrNoError)
        {
            co_return {true, err};
        }
    }

    if (err != ErrNoError)
    {
        co_return {true, ErrTxnUnableToParseResponse};
    }

    if (response.m_response->m_err == ErrNoError)
    {
        LOG_DBG("successful add-offset-to-txn with group %s, transactional_id: %s",
                groupId.c_str(), m_transactional_id.c_str());
        co_return {false, ErrNoError};
    }

    switch (response.m_response->m_err)
    {
    case ErrConsumerCoordinatorNotAvailable:
    case ErrNotCoordinatorForConsumer:
        coordinator->Close();
        co_await m_client->RefreshTransactionCoordinator(m_transactional_id);
        [[fallthrough]];
    case ErrOffsetsLoadInProgress:
    case ErrConcurrentTransactions:
        co_return {true, response.m_response->m_err};
    case ErrUnknownProducerID:
    case ErrInvalidProducerIDMapping:
        co_return {false, abortable_error_if_possible(response.m_response->m_err)};
    case ErrGroupAuthorizationFailed:
    {
        Result result;
        result.Retry = false;
        result.Error = transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagAbortableError), response.m_response->m_err);
        co_return result;
    }
    default:
    {
        Result result;
        result.Retry = false;
        result.Error = transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagFatalError), response.m_response->m_err);
        co_return result;
    }
    }
}
coev::awaitable<TransactionManager::Result> TransactionManager::publish_offsets_to_txn_commit(const TopicPartitionOffsets &offsets, const std::string &groupId, TopicPartitionOffsets &out_offsets)
{
    std::shared_ptr<Broker> consumer_group_coordinator;
    int err = co_await m_client->GetCoordinator(groupId, consumer_group_coordinator);
    if (err != ErrNoError)
    {
        co_return {true, err};
    }

    auto request = std::make_shared<TxnOffsetCommitRequest>();
    request->m_transactional_id = m_transactional_id;
    request->m_producer_epoch = m_producer_epoch;
    request->m_producer_id = m_producer_id;
    request->m_group_id = groupId;
    request->m_topics = MapToRequest(offsets);

    if (m_client->GetConfig()->Version.IsAtLeast(V2_1_0_0))
    {
        request->m_version = 2;
    }
    else if (m_client->GetConfig()->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 1;
    }

    ResponsePromise<TxnOffsetCommitResponse> responses;
    err = co_await consumer_group_coordinator->TxnOffsetCommit(request, responses);
    if (err != ErrNoError)
    {
        consumer_group_coordinator->Close();
        m_client->RefreshCoordinator(groupId);
        co_return {true, err};
    }

    if (err != ErrNoError)
    {
        co_return {true, ErrTxnUnableToParseResponse};
    }

    std::vector<KError> response_errors;
    TopicPartitionOffsets failed_txn;

    for (auto &it : responses.m_response->m_topics)
    {
        auto &topic = it.first;
        auto &partitionErrors = it.second;

        for (auto &partitionError : partitionErrors)
        {
            switch (partitionError.m_err)
            {
            case ErrNoError:
                continue;
            case ErrRequestTimedOut:
            case ErrConsumerCoordinatorNotAvailable:
            case ErrNotCoordinatorForConsumer:
                consumer_group_coordinator->Close();
                m_client->RefreshCoordinator(groupId);
            case ErrUnknownTopicOrPartition:
            case ErrOffsetsLoadInProgress:
                break;
            case ErrIllegalGeneration:
            case ErrUnknownMemberId:
            case ErrFencedInstancedId:
            case ErrGroupAuthorizationFailed:
            {
                Result result;
                result.Retry = false;
                result.Error = transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagAbortableError), partitionError.m_err);
                co_return result;
            }
            default:
            {
                Result result;
                result.Retry = false;
                result.Error = transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagFatalError), partitionError.m_err);
                co_return result;
            }
            }
            TopicPartition tp(topic, partitionError.m_partition);
            failed_txn[tp] = offsets.at(tp);
            response_errors.push_back(partitionError.m_err);
        }
    }

    out_offsets = failed_txn;
    if (out_offsets.empty())
    {
        LOG_DBG("successful txn-offset-commit with group %s, transactional_id: %s",
                groupId.c_str(), m_transactional_id.c_str());
        co_return {false, ErrNoError};
    }
    co_return {true, ErrTxnOffsetCommit};
}
coev::awaitable<int> TransactionManager::publish_offsets_to_txn(const TopicPartitionOffsets &offsets, const std::string &groupId, TopicPartitionOffsets &out_offsets)
{
    int attempts_remaining = m_client->GetConfig()->Producer.Transaction.Retry.Max;
    int err = co_await Retry(
        attempts_remaining,
        [this, &groupId]() -> coev::awaitable<Result>
        { return publish_offsets_to_txn_add_offset(groupId); });
    if (err != ErrNoError)
    {
        out_offsets = offsets;
        co_return err;
    }

    TopicPartitionOffsets resultOffsets = offsets;
    attempts_remaining = m_client->GetConfig()->Producer.Transaction.Retry.Max;

    TopicPartitionOffsets finalOffsets;
    err = co_await Retry(
        attempts_remaining,
        [this, &offsets, &groupId, &finalOffsets]() -> coev::awaitable<Result>
        {
            return publish_offsets_to_txn_commit(offsets, groupId, finalOffsets);
        });
    if (err != ErrNoError)
    {
        out_offsets = offsets;
    }
    co_return err;
}

coev::awaitable<int> TransactionManager::init_producer_id(int64_t &producerID, int16_t &producerEpoch)
{
    bool isEpochBump = false;

    std::shared_ptr<InitProducerIDRequest> request;
    if (is_transactional())
    {
        request->m_transactional_id = m_transactional_id;
        request->m_transaction_timeout = m_transaction_timeout;
    }

    if (m_client->GetConfig()->Version.IsAtLeast(V2_5_0_0))
    {
        if (m_client->GetConfig()->Version.IsAtLeast(V2_7_0_0))
        {
            request->m_version = 4;
        }
        else
        {
            request->m_version = 3;
        }
        isEpochBump = (producerID != noProducerID) && (producerEpoch != noProducerEpoch);
        m_coordinator_supports_bumping_epoch = true;
        request->m_producer_id = producerID;
        request->m_producer_epoch = producerEpoch;
    }
    else if (m_client->GetConfig()->Version.IsAtLeast(V2_4_0_0))
    {
        request->m_version = 2;
    }
    else if (m_client->GetConfig()->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 1;
    }

    int attempts_remaining = m_client->GetConfig()->Producer.Transaction.Retry.Max;
    co_return co_await Retry(
        attempts_remaining,
        [this, &isEpochBump, &request, &producerID, &producerEpoch]() -> coev::awaitable<Result>
        {
            int err = ErrNoError;
            std::shared_ptr<Broker> coordinator;
            if (is_transactional())
            {
                err = co_await m_client->TransactionCoordinator(m_transactional_id, coordinator);
                if (!coordinator)
                {
                    producerID = noProducerID;
                    producerEpoch = noProducerEpoch;
                    co_return {true, -1};
                }
            }
            else
            {
                err = co_await m_client->LeastLoadedBroker(coordinator);
                if (err != 0)
                {
                    producerID = noProducerID;
                    producerEpoch = noProducerEpoch;
                    co_return {true, -1};
                }
            }

            ResponsePromise<InitProducerIDResponse> response;
            err = co_await coordinator->InitProducerID(request, response);
            if (err != ErrNoError)
            {
                if (is_transactional())
                {
                    coordinator->Close();
                    co_await m_client->RefreshTransactionCoordinator(m_transactional_id);
                }
                producerID = noProducerID;
                producerEpoch = noProducerEpoch;
                co_return {true, err};
            }

            if (response.m_response->m_err == ErrNoError)
            {
                if (isEpochBump)
                {
                    m_sequence_numbers.clear();
                }
                err = transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagReady), ErrNoError);
                if (err != ErrNoError)
                {
                    producerID = noProducerID;
                    producerEpoch = noProducerEpoch;
                    co_return {true, err};
                }
                producerID = response.m_response->m_producer_id;
                producerEpoch = response.m_response->m_producer_epoch;
                co_return {false, ErrNoError};
            }
            switch (response.m_response->m_err)
            {
            case ErrConsumerCoordinatorNotAvailable:
            case ErrNotCoordinatorForConsumer:
            case ErrOffsetsLoadInProgress:
                if (is_transactional())
                {
                    coordinator->Close();
                    co_await m_client->RefreshTransactionCoordinator(m_transactional_id);
                }
                producerID = noProducerID;
                producerEpoch = noProducerEpoch;
                co_return {true, response.m_response->m_err};
            default:
                transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagFatalError), response.m_response->m_err);
                producerID = noProducerID;
                producerEpoch = noProducerEpoch;
                co_return {false, response.m_response->m_err};
            }
        });
}
int TransactionManager::abortable_error_if_possible(int err)
{
    if (m_coordinator_supports_bumping_epoch)
    {
        m_epoch_bump_required = true;
        return transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagAbortableError), err);
    }
    return transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagFatalError), err);
}

int TransactionManager::complete_transaction()
{
    if (m_epoch_bump_required)
    {
        int err = transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInitializing), 0);
        if (err != 0)
            return err;
    }
    else
    {
        int err = transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagReady), 0);
        if (err != 0)
            return err;
    }
    m_last_error = 0;
    m_epoch_bump_required = false;
    m_partitions_in_current_txn.clear();
    m_pending_partitions_in_current_txn.clear();
    m_offsets_in_current_txn.clear();
    return 0;
}

coev::awaitable<int> TransactionManager::end_txn(bool commit)
{
    int attempts_remaining = m_client->GetConfig()->Producer.Transaction.Retry.Max;
    return Retry(attempts_remaining,
                 [&]() -> coev::awaitable<Result>
                 {
                     std::shared_ptr<Broker> coordinator;
                     auto err = co_await m_client->TransactionCoordinator(m_transactional_id, coordinator);
                     if (err != ErrNoError)
                     {
                         co_return {true, -1};
                     }

                     co_return {false, 0};
                 });
}

coev::awaitable<int> TransactionManager::finish_transaction(bool commit)
{
    if (commit && (current_txn_status() & ProducerTxnFlagInError) != 0)
    {
        co_return m_last_error;
    }
    else if (!commit && (current_txn_status() & ProducerTxnFlagFatalError) != 0)
    {
        co_return m_last_error;
    }
    if (m_partitions_in_current_txn.empty())
    {
        co_return complete_transaction();
    }
    bool epochBump = m_epoch_bump_required;
    if (commit && !m_offsets_in_current_txn.empty())
    {
        for (auto it = m_offsets_in_current_txn.begin(); it != m_offsets_in_current_txn.end();)
        {
            TopicPartitionOffsets newOffsets;
            auto err = co_await publish_offsets_to_txn(it->second, it->first, newOffsets);
            if (err != 0)
            {
                it->second = newOffsets;
                co_return err;
            }
            it = m_offsets_in_current_txn.erase(it);
        }
    }
    if ((current_txn_status() & ProducerTxnFlagFatalError) != 0)
    {
        co_return m_last_error;
    }
    if (m_last_error != ErrInvalidProducerIDMapping)
    {
        int err = co_await end_txn(commit);
        if (err != 0)
        {
            co_return err;
        }
        if (!epochBump)
        {
            co_return 0;
        }
    }
    co_return co_await initialize_transactions();
}

void TransactionManager::maybe_add_partition_to_current_txn(const std::string &topic, int32_t partition)
{
    if ((current_txn_status() & ProducerTxnFlagInError) != 0)
    {
        return;
    }
    TopicPartition tp{topic, partition};
    if (m_partitions_in_current_txn.count(tp) > 0)
    {
        return;
    }
    m_pending_partitions_in_current_txn[tp] = {};
}

coev::awaitable<int> TransactionManager::publish_txn_partitions()
{
    if ((current_txn_status() & ProducerTxnFlagInError) != 0)
    {
        co_return m_last_error;
    }
    if (m_pending_partitions_in_current_txn.empty())
    {
        co_return 0;
    }
    auto removeAllPartitionsOnFatalOrAbortedError = [&]()
    {
        m_pending_partitions_in_current_txn.clear();
    };
    auto retryBackoff = m_client->GetConfig()->Producer.Transaction.Retry.Backoff;

    int attempts_remaining = m_client->GetConfig()->Producer.Transaction.Retry.Max;
    co_return co_await Retry(
        attempts_remaining,
        [&]() -> coev::awaitable<Result>
        {
            std::shared_ptr<Broker> coordinator;
            auto err = co_await m_client->TransactionCoordinator(m_transactional_id, coordinator);
            if (err != ErrNoError)
            {
                co_return {true, -1};
            }
            co_return {false, 0};
        });
}

coev::awaitable<int> TransactionManager::initialize_transactions()
{
    return init_producer_id(m_producer_id, m_producer_epoch);
}

TransactionManager::TransactionManager(std::shared_ptr<Config> conf, std::shared_ptr<Client> client)
    : m_producer_id(noProducerID),
      m_producer_epoch(noProducerEpoch),
      m_client(client),
      m_status(ProducerTxnFlagUninitialized)
{
    if (conf->Producer.Idempotent)
    {
        m_transactional_id = conf->Producer.Transaction.ID;
        m_transaction_timeout = conf->Producer.Transaction.Timeout;
        m_sequence_numbers = std::map<std::string, int32_t>();
    }
}
