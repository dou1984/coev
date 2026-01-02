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
        result[kv.first.topic].push_back(kv.first.partition);
    }
    return result;
}

std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> MapToRequest(const TopicPartitionOffsets &s)
{
    std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> result;
    for (auto &kv : s)
    {
        result[kv.first.topic].push_back(kv.second);
    }
    return result;
}

bool TransactionManager::IsTransitionValid(ProducerTxnStatusFlag target) const
{
    for (auto &it : producerTxnTransitions)
    {
        auto &status = it.first;
        auto &allowedTransitions = it.second;
        if ((status_ & status) != 0)
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
    std::shared_lock<std::shared_mutex> lock(statusLock_);
    return status_;
}

int TransactionManager::TransitionTo(ProducerTxnStatusFlag target, int err)
{
    std::unique_lock<std::shared_mutex> lock(statusLock_);
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
        lastError_ = err;
    }
    else
    {
        lastError_ = 0;
    }
    status_ = target;
    return err;
}

void TransactionManager::GetAndIncrementSequenceNumber(const std::string &topic, int32_t partition, int32_t &sequence, int16_t &epoch)
{
    std::string key = topic + "-" + std::to_string(partition);
    std::lock_guard<std::mutex> lock(mutex_);
    sequence = sequenceNumbers_[key];
    sequenceNumbers_[key] = sequence + 1;
    epoch = producerEpoch_;
}

void TransactionManager::BumpEpoch()
{
    std::lock_guard<std::mutex> lock(mutex_);
    producerEpoch_++;
    for (auto &kv : sequenceNumbers_)
    {
        kv.second = 0;
    }
}

std::pair<int64_t, int16_t> TransactionManager::GetProducerID()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return {producerID_, producerEpoch_};
}

std::chrono::milliseconds TransactionManager::ComputeBackoff(int attemptsRemaining) const
{
    auto config = client_->GetConfig();
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
    return !transactionalID_.empty();
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
        Logger::Printf("txnmgr [%s] retrying after %lldms %d attempts remaining %d\n",
                       transactionalID_.c_str(), static_cast<long long>(backoff.count() / 1000000), attemptsRemaining, static_cast<int>(r.Error));

        co_await sleep_for(backoff);
        attemptsRemaining--;
    }
    co_return 0;
}
int TransactionManager::AddOffsetsToTxn(const std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> &offsetsToAdd, const std::string &groupId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if ((CurrentTxnStatus() & ProducerTxnFlagInTransaction) == 0)
    {
        return ErrTransactionNotReady;
    }
    if ((CurrentTxnStatus() & ProducerTxnFlagFatalError) != 0)
    {
        return lastError_;
    }
    if (offsetsInCurrentTxn_.find(groupId) == offsetsInCurrentTxn_.end())
    {
        offsetsInCurrentTxn_[groupId] = TopicPartitionOffsets{};
    }
    for (auto &topicOffsets : offsetsToAdd)
    {
        const std::string &topic = topicOffsets.first;
        for (auto &offset : topicOffsets.second)
        {
            TopicPartition tp{topic, offset->Partition};
            offsetsInCurrentTxn_[groupId][tp] = offset;
        }
    }
    return 0;
}

coev::awaitable<TransactionManager::Result> TransactionManager::PublishOffsetsToTxnAddOffset(const std::string &groupId)
{
    std::shared_ptr<Broker> coordinator;
    int err = co_await client_->TransactionCoordinator(transactionalID_, coordinator);
    if (err != ErrNoError)
    {
        co_return {true, err};
    }

    auto request = std::make_shared<AddOffsetsToTxnRequest>();
    request->TransactionalID = transactionalID_;
    request->ProducerEpoch = producerEpoch_;
    request->ProducerID = producerID_;
    request->GroupID = groupId;

    if (client_->GetConfig()->Version.IsAtLeast(V2_7_0_0))
    {
        request->Version = 2;
    }
    else if (client_->GetConfig()->Version.IsAtLeast(V2_0_0_0))
    {
        request->Version = 1;
    }

    std::shared_ptr<AddOffsetsToTxnResponse> response;
    err = co_await coordinator->AddOffsetsToTxn(request, response);
    if (err != ErrNoError)
    {
        co_await coordinator->Close();
        err = co_await client_->RefreshTransactionCoordinator(transactionalID_);
        if (err != ErrNoError)
        {
            co_return {true, err};
        }
    }

    if (!response)
    {
        co_return {true, ErrTxnUnableToParseResponse};
    }

    if (response->Err == ErrNoError)
    {
        DebugLogger::Printf("txnmgr/add-offset-to-txn [%s] successful add-offset-to-txn with group %s\n",
                            transactionalID_.c_str(), groupId.c_str());
        co_return {false, ErrNoError};
    }

    switch (response->Err)
    {
    case ErrConsumerCoordinatorNotAvailable:
    case ErrNotCoordinatorForConsumer:
        coordinator->Close();
        co_await client_->RefreshTransactionCoordinator(transactionalID_);
        [[fallthrough]];
    case ErrOffsetsLoadInProgress:
    case ErrConcurrentTransactions:
        co_return {true, response->Err};
    case ErrUnknownProducerID:
    case ErrInvalidProducerIDMapping:
        co_return {false, AbortableErrorIfPossible(response->Err)};
    case ErrGroupAuthorizationFailed:
        co_return {false, TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagAbortableError), response->Err)};
    default:
        co_return {false, TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagFatalError), response->Err)};
    }
}
coev::awaitable<TransactionManager::Result> TransactionManager::PublishOffsetsToTxnCommit(const TopicPartitionOffsets &offsets, const std::string &groupId, TopicPartitionOffsets &outOffsets)
{
    std::shared_ptr<Broker> consumerGroupCoordinator;
    int err = co_await client_->GetCoordinator(groupId, consumerGroupCoordinator);
    if (err != ErrNoError)
    {
        co_return {true, err};
    }

    auto request = std::make_shared<TxnOffsetCommitRequest>();
    request->TransactionalID = transactionalID_;
    request->ProducerEpoch = producerEpoch_;
    request->ProducerID = producerID_;
    request->GroupID = groupId;
    request->Topics = MapToRequest(offsets);

    if (client_->GetConfig()->Version.IsAtLeast(V2_1_0_0))
    {
        request->Version = 2;
    }
    else if (client_->GetConfig()->Version.IsAtLeast(V2_0_0_0))
    {
        request->Version = 1;
    }

    std::shared_ptr<TxnOffsetCommitResponse> responses;
    err = co_await consumerGroupCoordinator->TxnOffsetCommit(request, responses);
    if (err != ErrNoError)
    {
        consumerGroupCoordinator->Close();
        client_->RefreshCoordinator(groupId);
        co_return {true, err};
    }

    if (!responses)
    {
        co_return {true, ErrTxnUnableToParseResponse};
    }

    std::vector<KError> responseErrors;
    TopicPartitionOffsets failedTxn;

    for (auto &it : responses->Topics)
    {
        auto &topic = it.first;
        auto &partitionErrors = it.second;

        for (auto &partitionError : partitionErrors)
        {
            switch (partitionError->Err)
            {
            case ErrNoError:
                continue;
            case ErrRequestTimedOut:
            case ErrConsumerCoordinatorNotAvailable:
            case ErrNotCoordinatorForConsumer:
                consumerGroupCoordinator->Close();
                client_->RefreshCoordinator(groupId);
                [[fallthrough]];
            case ErrUnknownTopicOrPartition:
            case ErrOffsetsLoadInProgress:
                break;
            case ErrIllegalGeneration:
            case ErrUnknownMemberId:
            case ErrFencedInstancedId:
            case ErrGroupAuthorizationFailed:
                co_return {false, TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagAbortableError), partitionError->Err)};
            default:
                co_return {false, TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagFatalError), partitionError->Err)};
            }
            TopicPartition tp{topic, partitionError->Partition};
            failedTxn[tp] = offsets.at(tp);
            responseErrors.push_back(partitionError->Err);
        }
    }

    outOffsets = failedTxn;
    if (outOffsets.empty())
    {
        DebugLogger::Printf("txnmgr/txn-offset-commit [%s] successful txn-offset-commit with group %s\n",
                            transactionalID_.c_str(), groupId.c_str());
        co_return {false, ErrNoError};
    }
    co_return {true, ErrTxnOffsetCommit};
}
coev::awaitable<int> TransactionManager::PublishOffsetsToTxn(const TopicPartitionOffsets &offsets, const std::string &groupId, TopicPartitionOffsets &outOffsets)
{
    int attemptsRemaining = client_->GetConfig()->Producer.Transaction.Retry.Max;
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
    attemptsRemaining = client_->GetConfig()->Producer.Transaction.Retry.Max;

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
        request->TransactionalID = transactionalID_;
        request->TransactionTimeout = transactionTimeout_;
    }

    if (client_->GetConfig()->Version.IsAtLeast(V2_5_0_0))
    {
        if (client_->GetConfig()->Version.IsAtLeast(V2_7_0_0))
        {
            request->Version = 4;
        }
        else
        {
            request->Version = 3;
        }
        isEpochBump = (producerID != noProducerID) && (producerEpoch != noProducerEpoch);
        coordinatorSupportsBumpingEpoch_ = true;
        request->ProducerID = producerID;
        request->ProducerEpoch = producerEpoch;
    }
    else if (client_->GetConfig()->Version.IsAtLeast(V2_4_0_0))
    {
        request->Version = 2;
    }
    else if (client_->GetConfig()->Version.IsAtLeast(V2_0_0_0))
    {
        request->Version = 1;
    }

    int attemptsRemaining = client_->GetConfig()->Producer.Transaction.Retry.Max;
    co_return co_await Retry(
        attemptsRemaining,
        [this, &isEpochBump, &request, &producerID, &producerEpoch]() -> coev::awaitable<Result>
        {
            int err = ErrNoError;
            std::shared_ptr<Broker> coordinator;
            if (IsTransactional())
            {
                err = co_await client_->TransactionCoordinator(transactionalID_, coordinator);
                if (!coordinator)
                {
                    producerID = noProducerID;
                    producerEpoch = noProducerEpoch;
                    co_return {true, -1};
                }
            }
            else
            {
                err = co_await client_->LeastLoadedBroker(coordinator);
                if (err != 0)
                {
                    producerID = noProducerID;
                    producerEpoch = noProducerEpoch;
                    co_return {true, -1};
                }
            }

            std::shared_ptr<InitProducerIDResponse> response;
            err = co_await coordinator->InitProducerID(request, response);
            if (!response)
            {
                if (IsTransactional())
                {
                    co_await coordinator->Close();
                    co_await client_->RefreshTransactionCoordinator(transactionalID_);
                }
                producerID = noProducerID;
                producerEpoch = noProducerEpoch;
                co_return {true, err};
            }

            if (response->Err == ErrNoError)
            {
                if (isEpochBump)
                {
                    sequenceNumbers_.clear();
                }
                err = TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagReady), ErrNoError);
                if (err != ErrNoError)
                {
                    producerID = noProducerID;
                    producerEpoch = noProducerEpoch;
                    co_return {true, err};
                }
                producerID = response->ProducerID;
                producerEpoch = response->ProducerEpoch;
                co_return {false, ErrNoError};
            }

            switch (response->Err)
            {
            case ErrConsumerCoordinatorNotAvailable:
            case ErrNotCoordinatorForConsumer:
            case ErrOffsetsLoadInProgress:
                if (IsTransactional())
                {
                    co_await coordinator->Close();
                    co_await client_->RefreshTransactionCoordinator(transactionalID_);
                }
                producerID = response->ProducerID;
                producerEpoch = response->ProducerEpoch;
                co_return {true, response->Err};
            default:
                TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagFatalError), response->Err);
                producerID = response->ProducerID;
                producerEpoch = response->ProducerEpoch;
                co_return {false, response->Err};
            }
        });
}
int TransactionManager::AbortableErrorIfPossible(int err)
{
    if (coordinatorSupportsBumpingEpoch_)
    {
        epochBumpRequired_ = true;
        return TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagAbortableError), err);
    }
    return TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagFatalError), err);
}

int TransactionManager::CompleteTransaction()
{
    if (epochBumpRequired_)
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
    lastError_ = 0;
    epochBumpRequired_ = false;
    partitionsInCurrentTxn_.clear();
    pendingPartitionsInCurrentTxn_.clear();
    offsetsInCurrentTxn_.clear();
    return 0;
}

coev::awaitable<int> TransactionManager::EndTxn(bool commit)
{
    int attemptsRemaining = client_->GetConfig()->Producer.Transaction.Retry.Max;
    return Retry(attemptsRemaining,
                 [&]() -> coev::awaitable<Result>
                 {
                     std::shared_ptr<Broker> coordinator;
                     auto err = co_await client_->TransactionCoordinator(transactionalID_, coordinator);
                     if (err != ErrNoError)
                     {
                         co_return {true, -1};
                     }

                     co_return {false, 0};
                 });
}

coev::awaitable<int> TransactionManager::FinishTransaction(bool commit)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (commit && (CurrentTxnStatus() & ProducerTxnFlagInError) != 0)
    {
        co_return lastError_;
    }
    else if (!commit && (CurrentTxnStatus() & ProducerTxnFlagFatalError) != 0)
    {
        co_return lastError_;
    }
    if (partitionsInCurrentTxn_.empty())
    {
        co_return CompleteTransaction();
    }
    bool epochBump = epochBumpRequired_;
    if (commit && !offsetsInCurrentTxn_.empty())
    {
        for (auto it = offsetsInCurrentTxn_.begin(); it != offsetsInCurrentTxn_.end();)
        {
            TopicPartitionOffsets newOffsets;
            auto err = co_await PublishOffsetsToTxn(it->second, it->first, newOffsets);
            if (err != 0)
            {
                it->second = newOffsets;
                co_return err;
            }
            it = offsetsInCurrentTxn_.erase(it);
        }
    }
    if ((CurrentTxnStatus() & ProducerTxnFlagFatalError) != 0)
    {
        co_return lastError_;
    }
    if (lastError_ != ErrInvalidProducerIDMapping)
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
    std::lock_guard<std::mutex> lock(partitionInTxnLock_);
    if (partitionsInCurrentTxn_.count(tp) > 0)
    {
        return;
    }
    pendingPartitionsInCurrentTxn_[tp] = {};
}

coev::awaitable<int> TransactionManager::PublishTxnPartitions()
{
    std::lock_guard<std::mutex> lock(partitionInTxnLock_);
    if ((CurrentTxnStatus() & ProducerTxnFlagInError) != 0)
    {
        co_return lastError_;
    }
    if (pendingPartitionsInCurrentTxn_.empty())
    {
        co_return 0;
    }
    auto removeAllPartitionsOnFatalOrAbortedError = [&]()
    {
        pendingPartitionsInCurrentTxn_.clear();
    };
    auto retryBackoff = client_->GetConfig()->Producer.Transaction.Retry.Backoff;

    int attemptsRemaining = client_->GetConfig()->Producer.Transaction.Retry.Max;
    co_return co_await Retry(
        attemptsRemaining,
        [&]() -> coev::awaitable<Result>
        {
            std::shared_ptr<Broker> coordinator;
            auto err = co_await client_->TransactionCoordinator(transactionalID_, coordinator);
            if (err != ErrNoError)
            {
                co_return {true, -1};
            }
            co_return {false, 0};
        });
}

coev::awaitable<int> TransactionManager::InitializeTransactions()
{
    return InitProducerId(producerID_, producerEpoch_);
}

coev::awaitable<int> NewTransactionManager(std::shared_ptr<Config> conf, std::shared_ptr<Client> client, std::shared_ptr<TransactionManager> &txnmgr)
{
    txnmgr = std::make_shared<TransactionManager>();
    txnmgr->producerID_ = noProducerID;
    txnmgr->producerEpoch_ = noProducerEpoch;
    txnmgr->client_ = client;
    txnmgr->status_ = ProducerTxnFlagUninitialized;
    if (conf->Producer.Idempotent)
    {
        txnmgr->transactionalID_ = conf->Producer.Transaction.ID;
        txnmgr->transactionTimeout_ = conf->Producer.Transaction.Timeout;
        txnmgr->sequenceNumbers_ = std::map<std::string, int32_t>();
        co_return co_await txnmgr->InitProducerId(txnmgr->producerID_, txnmgr->producerEpoch_);
    }
    co_return 0;
}
