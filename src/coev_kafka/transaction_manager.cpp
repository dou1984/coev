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

std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> MapToRequest(const TopicPartitionOffsets &s)
{
    std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> result;
    for (auto &kv : s)
    {
        result[kv.first.m_topic].push_back(kv.second);
    }
    return result;
}

bool TransactionManager::IsTransitionValid(ProducerTxnStatusFlag target) const
{
    for (auto &it : producerTxnTransitions)
    {
        auto &status = it.first;
        auto &allowedTransitions = it.second;
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

ProducerTxnStatusFlag TransactionManager::CurrentTxnStatus() const
{
    std::shared_lock<std::shared_mutex> lock(m_status_lock);
    return m_status;
}

int TransactionManager::TransitionTo(ProducerTxnStatusFlag target, int err)
{
    std::unique_lock<std::shared_mutex> lock(m_status_lock);
    if (!IsTransitionValid(target))
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

void TransactionManager::GetAndIncrementSequenceNumber(const std::string &topic, int32_t partition, int32_t &sequence, int16_t &epoch)
{
    std::string key = topic + "-" + std::to_string(partition);
    std::lock_guard<std::mutex> lock(m_mutex);
    sequence = m_sequence_numbers[key];
    m_sequence_numbers[key] = sequence + 1;
    epoch = m_producer_epoch;
}

void TransactionManager::BumpEpoch()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_producer_epoch++;
    for (auto &kv : m_sequence_numbers)
    {
        kv.second = 0;
    }
}

std::pair<int64_t, int16_t> TransactionManager::GetProducerID()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return {m_producer_id, m_producer_epoch};
}

std::chrono::milliseconds TransactionManager::ComputeBackoff(int attemptsRemaining) const
{
    auto config = m_client->GetConfig();
    if (config->Producer.Transaction.Retry.BackoffFunc)
    {
        int maxRetries = config->Producer.Transaction.Retry.Max;
        int retries = maxRetries - attemptsRemaining;
        return config->Producer.Transaction.Retry.BackoffFunc(retries, maxRetries);
    }
    return config->Producer.Transaction.Retry.Backoff;
}

bool TransactionManager::IsTransactional() const
{
    return !m_transactional_id.empty();
}
coev::awaitable<int> TransactionManager::Retry(int attemptsRemaining, const std::function<coev::awaitable<TransactionManager::Result>()> &run)
{
    while (attemptsRemaining >= 0)
    {
        auto r = co_await run();
        if (!r.Retry)
        {
            co_return r.Error;
        }
        auto backoff = ComputeBackoff(attemptsRemaining);
        LOG_CORE("retrying after %lldms %d attempts remaining %d",
                 static_cast<long long>(backoff.count() / 1000000), attemptsRemaining, static_cast<int>(r.Error));

        co_await sleep_for(backoff);
        attemptsRemaining--;
    }
    co_return 0;
}
int TransactionManager::AddOffsetsToTxn(const std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> &offsetsToAdd, const std::string &groupId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if ((CurrentTxnStatus() & ProducerTxnFlagInTransaction) == 0)
    {
        return ErrTransactionNotReady;
    }
    if ((CurrentTxnStatus() & ProducerTxnFlagFatalError) != 0)
    {
        return m_last_error;
    }
    if (m_offsets_in_current_txn.find(groupId) == m_offsets_in_current_txn.end())
    {
        m_offsets_in_current_txn[groupId] = TopicPartitionOffsets{};
    }
    for (auto &topicOffsets : offsetsToAdd)
    {
        const std::string &topic = topicOffsets.first;
        for (auto &offset : topicOffsets.second)
        {
            TopicPartition tp(topic, offset->m_partition);
            m_offsets_in_current_txn[groupId][tp] = offset;
        }
    }
    return 0;
}

coev::awaitable<TransactionManager::Result> TransactionManager::PublishOffsetsToTxnAddOffset(const std::string &groupId)
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
        co_return {false, AbortableErrorIfPossible(response.m_response->m_err)};
    case ErrGroupAuthorizationFailed:
        co_return {false, TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagAbortableError), response.m_response->m_err)};
    default:
        co_return {false, TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagFatalError), response.m_response->m_err)};
    }
}
coev::awaitable<TransactionManager::Result> TransactionManager::PublishOffsetsToTxnCommit(const TopicPartitionOffsets &offsets, const std::string &groupId, TopicPartitionOffsets &outOffsets)
{
    std::shared_ptr<Broker> consumerGroupCoordinator;
    int err = co_await m_client->GetCoordinator(groupId, consumerGroupCoordinator);
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
    err = co_await consumerGroupCoordinator->TxnOffsetCommit(request, responses);
    if (err != ErrNoError)
    {
        consumerGroupCoordinator->Close();
        m_client->RefreshCoordinator(groupId);
        co_return {true, err};
    }

    if (err != ErrNoError)
    {
        co_return {true, ErrTxnUnableToParseResponse};
    }

    std::vector<KError> responseErrors;
    TopicPartitionOffsets failedTxn;

    for (auto &it : responses.m_response->m_topics)
    {
        auto &topic = it.first;
        auto &partitionErrors = it.second;

        for (auto &partitionError : partitionErrors)
        {
            switch (partitionError->m_err)
            {
            case ErrNoError:
                continue;
            case ErrRequestTimedOut:
            case ErrConsumerCoordinatorNotAvailable:
            case ErrNotCoordinatorForConsumer:
                consumerGroupCoordinator->Close();
                m_client->RefreshCoordinator(groupId);
            case ErrUnknownTopicOrPartition:
            case ErrOffsetsLoadInProgress:
                break;
            case ErrIllegalGeneration:
            case ErrUnknownMemberId:
            case ErrFencedInstancedId:
            case ErrGroupAuthorizationFailed:
                co_return {false, TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagAbortableError), partitionError->m_err)};
            default:
                co_return {false, TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagFatalError), partitionError->m_err)};
            }
            TopicPartition tp(topic, partitionError->m_partition);
            failedTxn[tp] = offsets.at(tp);
            responseErrors.push_back(partitionError->m_err);
        }
    }

    outOffsets = failedTxn;
    if (outOffsets.empty())
    {
        LOG_DBG("successful txn-offset-commit with group %s, transactional_id: %s",
                groupId.c_str(), m_transactional_id.c_str());
        co_return {false, ErrNoError};
    }
    co_return {true, ErrTxnOffsetCommit};
}
coev::awaitable<int> TransactionManager::PublishOffsetsToTxn(const TopicPartitionOffsets &offsets, const std::string &groupId, TopicPartitionOffsets &outOffsets)
{
    int attemptsRemaining = m_client->GetConfig()->Producer.Transaction.Retry.Max;
    int err = co_await Retry(
        attemptsRemaining,
        [this, &groupId]() -> coev::awaitable<Result>
        { return PublishOffsetsToTxnAddOffset(groupId); });
    if (err != ErrNoError)
    {
        outOffsets = offsets;
        co_return err;
    }

    TopicPartitionOffsets resultOffsets = offsets;
    attemptsRemaining = m_client->GetConfig()->Producer.Transaction.Retry.Max;

    TopicPartitionOffsets finalOffsets;
    err = co_await Retry(
        attemptsRemaining,
        [this, &offsets, &groupId, &finalOffsets]() -> coev::awaitable<Result>
        {
            return PublishOffsetsToTxnCommit(offsets, groupId, finalOffsets);
        });
    if (err != ErrNoError)
    {
        outOffsets = offsets;
    }
    co_return err;
}

coev::awaitable<int> TransactionManager::InitProducerId(int64_t &producerID, int16_t &producerEpoch)
{
    bool isEpochBump = false;

    std::shared_ptr<InitProducerIDRequest> request;
    if (IsTransactional())
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

    int attemptsRemaining = m_client->GetConfig()->Producer.Transaction.Retry.Max;
    co_return co_await Retry(
        attemptsRemaining,
        [this, &isEpochBump, &request, &producerID, &producerEpoch]() -> coev::awaitable<Result>
        {
            int err = ErrNoError;
            std::shared_ptr<Broker> coordinator;
            if (IsTransactional())
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
                if (IsTransactional())
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
                err = TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagReady), ErrNoError);
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
                if (IsTransactional())
                {
                    coordinator->Close();
                    co_await m_client->RefreshTransactionCoordinator(m_transactional_id);
                }
                producerID = noProducerID;
                producerEpoch = noProducerEpoch;
                co_return {true, response.m_response->m_err};
            default:
                TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagFatalError), response.m_response->m_err);
                producerID = noProducerID;
                producerEpoch = noProducerEpoch;
                co_return {false, response.m_response->m_err};
            }
        });
}
int TransactionManager::AbortableErrorIfPossible(int err)
{
    if (m_coordinator_supports_bumping_epoch)
    {
        m_epoch_bump_required = true;
        return TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagAbortableError), err);
    }
    return TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagFatalError), err);
}

int TransactionManager::CompleteTransaction()
{
    if (m_epoch_bump_required)
    {
        int err = TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInitializing), 0);
        if (err != 0)
            return err;
    }
    else
    {
        int err = TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagReady), 0);
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

coev::awaitable<int> TransactionManager::EndTxn(bool commit)
{
    int attemptsRemaining = m_client->GetConfig()->Producer.Transaction.Retry.Max;
    return Retry(attemptsRemaining,
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

coev::awaitable<int> TransactionManager::FinishTransaction(bool commit)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (commit && (CurrentTxnStatus() & ProducerTxnFlagInError) != 0)
    {
        co_return m_last_error;
    }
    else if (!commit && (CurrentTxnStatus() & ProducerTxnFlagFatalError) != 0)
    {
        co_return m_last_error;
    }
    if (m_partitions_in_current_txn.empty())
    {
        co_return CompleteTransaction();
    }
    bool epochBump = m_epoch_bump_required;
    if (commit && !m_offsets_in_current_txn.empty())
    {
        for (auto it = m_offsets_in_current_txn.begin(); it != m_offsets_in_current_txn.end();)
        {
            TopicPartitionOffsets newOffsets;
            auto err = co_await PublishOffsetsToTxn(it->second, it->first, newOffsets);
            if (err != 0)
            {
                it->second = newOffsets;
                co_return err;
            }
            it = m_offsets_in_current_txn.erase(it);
        }
    }
    if ((CurrentTxnStatus() & ProducerTxnFlagFatalError) != 0)
    {
        co_return m_last_error;
    }
    if (m_last_error != ErrInvalidProducerIDMapping)
    {
        int err = co_await EndTxn(commit);
        if (err != 0)
            co_return err;
        if (!epochBump)
            co_return 0;
    }
    co_return co_await InitializeTransactions();
}

void TransactionManager::MaybeAddPartitionToCurrentTxn(const std::string &topic, int32_t partition)
{
    if ((CurrentTxnStatus() & ProducerTxnFlagInError) != 0)
    {
        return;
    }
    TopicPartition tp{topic, partition};
    std::lock_guard<std::mutex> lock(m_partition_in_txn_lock);
    if (m_partitions_in_current_txn.count(tp) > 0)
    {
        return;
    }
    m_pending_partitions_in_current_txn[tp] = {};
}

coev::awaitable<int> TransactionManager::PublishTxnPartitions()
{
    std::lock_guard<std::mutex> lock(m_partition_in_txn_lock);
    if ((CurrentTxnStatus() & ProducerTxnFlagInError) != 0)
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

    int attemptsRemaining = m_client->GetConfig()->Producer.Transaction.Retry.Max;
    co_return co_await Retry(
        attemptsRemaining,
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

coev::awaitable<int> TransactionManager::InitializeTransactions()
{
    return InitProducerId(m_producer_id, m_producer_epoch);
}

coev::awaitable<int> NewTransactionManager(std::shared_ptr<Config> conf, std::shared_ptr<Client> client, std::shared_ptr<TransactionManager> &txnmgr)
{
    txnmgr = std::make_shared<TransactionManager>();
    txnmgr->m_producer_id = noProducerID;
    txnmgr->m_producer_epoch = noProducerEpoch;
    txnmgr->m_client = client;
    txnmgr->m_status = ProducerTxnFlagUninitialized;
    if (conf->Producer.Idempotent)
    {
        txnmgr->m_transactional_id = conf->Producer.Transaction.ID;
        txnmgr->m_transaction_timeout = conf->Producer.Transaction.Timeout;
        txnmgr->m_sequence_numbers = std::map<std::string, int32_t>();
        co_return co_await txnmgr->InitProducerId(txnmgr->m_producer_id, txnmgr->m_producer_epoch);
    }
    co_return 0;
}
