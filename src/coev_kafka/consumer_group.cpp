#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include "version.h"
#include "errors.h"
#include "metrics.h"
#include "consumer.h"
#include "sleep_for.h"
#include "offset_manager.h"
#include "partition_offset_manager.h"
#include "consumer_group.h"
#include "consumer_group_session.h"
#include "consumer_group_claim.h"
#include "consumer_group_handler.h"

ConsumerGroup::ConsumerGroup(std::shared_ptr<IClient> client, std::shared_ptr<IConsumer> consumer, std::shared_ptr<Config> config, const std::string &group_)
    : client_(client), config_(config), consumer_(consumer), groupId(group_)
{

    userData = config_->Consumer.Group.Member.UserData;

    if (!config_->Version.IsAtLeast(V0_10_2_0))
    {
        throw std::runtime_error("consumer groups require Version to be >= V0_10_2_0");
    }

    if (config_->Consumer.Group.InstanceID != "" && config_->Version.IsAtLeast(V2_3_0_0))
    {
        groupInstanceId = config_->Consumer.Group.InstanceID;
    }
}

coev::awaitable<int> ConsumerGroup::Consume(std::shared_ptr<Context> &ctx, const std::vector<std::string> &topics, std::shared_ptr<ConsumerGroupHandler> handler)
{
    bool isClosed = co_await closed.get();
    if (isClosed)
    {
        co_return ErrorGroupClosed;
    }

    std::lock_guard<std::mutex> lock_guard(lock);
    if (topics.empty())
    {
        co_return ErrTopicsProvided;
    }

    if (int err = co_await client_->RefreshMetadata(topics); err != 0)
    {
        co_return err;
    }
    std::shared_ptr<ConsumerGroupSession> session;
    auto err = co_await NewSession(ctx, topics, handler, config_->Consumer.Group.Rebalance.Retry.Max, session);
    if (err != ErrNoError)
    {
        co_return err;
    }

    co_return co_await session->Release_(true);
}

coev::co_channel<std::shared_ptr<ConsumerError>> &ConsumerGroup::Errors()
{
    return errors;
}

coev::awaitable<int> ConsumerGroup::Close()
{
    auto isClosed = co_await closed.get();
    if (isClosed)
    {
        co_return 0;
    }
    int err = co_await Leave();
    if (err != ErrNoError)
    {
        co_return err;
    }
    auto cErr = std::make_shared<ConsumerError>();
    while (errors.try_get(cErr))
    {
    }

    if (err = client_->Close(); err != 0)
    {
        co_return err;
    }

    co_return err;
}

void ConsumerGroup::Pause(const std::map<std::string, std::vector<int32_t>> &partitions)
{
    consumer_->Pause(partitions);
}

void ConsumerGroup::Resume(const std::map<std::string, std::vector<int32_t>> &partitions)
{
    consumer_->Resume(partitions);
}

void ConsumerGroup::PauseAll()
{
    consumer_->PauseAll();
}

void ConsumerGroup::ResumeAll()
{
    consumer_->ResumeAll();
}

coev::awaitable<int> ConsumerGroup::RetryNewSession(std::shared_ptr<Context> &ctx, const std::vector<std::string> &topics, std::shared_ptr<ConsumerGroupHandler> handler, int retries, bool refreshCoordinator, std::shared_ptr<ConsumerGroupSession> &out)
{
    auto select_result = co_await coev::wait_for_any(
        [ctx]() -> coev::awaitable<void>
        {
            co_await ctx->ch.get();
            co_return;
        }(),
        [this]() -> coev::awaitable<void>
        {
            co_await closed.get();
            co_return;
        }(),
        [this]() -> coev::awaitable<void>
        {
            co_await sleep_for(config_->Consumer.Group.Rebalance.Retry.Backoff);
            co_return;
        }());
    if (select_result == 0)
    {
        co_return 0;
    }
    else if (select_result == 1)
    {
        co_return ErrClosedClient;
    }

    if (refreshCoordinator)
    {
        auto err = co_await client_->RefreshCoordinator(groupId);
        if (err != ErrNoError)
        {
            if (retries <= 0)
            {
                co_return err;
            }
            err = co_await RetryNewSession(ctx, topics, handler, retries - 1, true, out);
            co_return err;
        }
    }

    auto err = co_await NewSession(ctx, topics, handler, retries - 1, out);
    co_return err;
}
coev::awaitable<int> ConsumerGroup::NewSession(std::shared_ptr<Context> &ctx, const std::vector<std::string> &topics, std::shared_ptr<ConsumerGroupHandler> handler, int retries, std::shared_ptr<ConsumerGroupSession> &session)
{

    int err = co_await client_->GetCoordinator(groupId, coordinator);
    if (err != 0)
    {
        if (retries <= 0)
        {
            co_return err;
        }
        co_return co_await RetryNewSession(ctx, topics, handler, retries, true, session);
    }

    std::shared_ptr<metrics::Counter> consumerGroupJoinTotal;
    std::shared_ptr<metrics::Counter> consumerGroupJoinFailed;
    std::shared_ptr<metrics::Counter> consumerGroupSyncTotal;
    std::shared_ptr<metrics::Counter> consumerGroupSyncFailed;

    if (metricRegistry != nullptr)
    {
        consumerGroupJoinTotal = metrics::GetOrRegisterCounter("consumer-group-join-total-" + groupId, metricRegistry);
        consumerGroupJoinFailed = metrics::GetOrRegisterCounter("consumer-group-join-failed-" + groupId, metricRegistry);
        consumerGroupSyncTotal = metrics::GetOrRegisterCounter("consumer-group-sync-total-" + groupId, metricRegistry);
        consumerGroupSyncFailed = metrics::GetOrRegisterCounter("consumer-group-sync-failed-" + groupId, metricRegistry);
    }
    std::shared_ptr<JoinGroupResponse> response;
    err = co_await JoinGroup(coordinator, topics, response);
    if (consumerGroupJoinTotal != nullptr)
    {
        consumerGroupJoinTotal->Inc(1);
    }
    if (err != 0)
    {
        co_await coordinator->Close();
        if (consumerGroupJoinFailed != nullptr)
        {
            consumerGroupJoinFailed->Inc(1);
        }
        co_return err;
    }

    if (response->Err != 0)
    {
        if (consumerGroupJoinFailed != nullptr)
        {
            consumerGroupJoinFailed->Inc(1);
        }
    }

    switch (response->Err)
    {
    case ErrNoError:
        memberId = response->MemberId;
        break;
    case ErrUnknownMemberId:
    case ErrIllegalGeneration:
        memberId = "";
        co_return co_await NewSession(ctx, topics, handler, retries, session);
    case ErrNotCoordinatorForConsumer:
    case ErrRebalanceInProgress:
    case ErrOffsetsLoadInProgress:
        if (retries <= 0)
        {
            co_return response->Err;
        }
        co_return co_await RetryNewSession(ctx, topics, handler, retries, true, session);
    case ErrMemberIdRequired:
        memberId = response->MemberId;
        co_return co_await NewSession(ctx, topics, handler, retries, session);
    case ErrFencedInstancedId:
        Logger::Printf("JoinGroup failed: group instance id %s has been fenced\n", groupInstanceId);
        co_return response->Err;
    default:
        co_return response->Err;
    }

    std::shared_ptr<BalanceStrategy> strategy = config_->Consumer.Group.Rebalance.Strategy;
    bool ok = false;
    if (strategy == nullptr)
    {
        strategy = findStrategy(response->GroupProtocol, config_->Consumer.Group.Rebalance.GroupStrategies, ok);
        if (!ok)
        {
            co_return ErrStrategyNotFound;
        }
    }

    std::map<std::string, ConsumerGroupMemberMetadata> members;
    std::map<std::string, std::vector<int32_t>> allSubscribedTopicPartitions;
    std::vector<std::string> allSubscribedTopics;
    BalanceStrategyPlan plan;

    if (response->LeaderId == response->MemberId)
    {
        err = response->GetMembers(members);
        if (err != 0)
        {
            co_return err;
        }
        err = co_await Balance(strategy, members, allSubscribedTopicPartitions, allSubscribedTopics, plan);
        if (err != 0)
        {
            co_return err;
        }
    }

    std::shared_ptr<SyncGroupResponse> syncGroupResponse;
    err = co_await SyncGroup(coordinator, members, plan, response->GenerationId, strategy, syncGroupResponse);
    if (consumerGroupSyncTotal != nullptr)
    {
        consumerGroupSyncTotal->Inc(1);
    }
    if (err != 0)
    {
        co_await coordinator->Close();
        if (consumerGroupSyncFailed != nullptr)
        {
            consumerGroupSyncFailed->Inc(1);
        }
        co_return err;
    }

    if (syncGroupResponse->Err != 0)
    {
        if (consumerGroupSyncFailed != nullptr)
        {
            consumerGroupSyncFailed->Inc(1);
        }
    }

    switch (syncGroupResponse->Err)
    {
    case ErrNoError:
        break;
    case ErrUnknownMemberId:
    case ErrIllegalGeneration:
        memberId = "";
        co_return co_await NewSession(ctx, topics, handler, retries, session);
    case ErrNotCoordinatorForConsumer:
    case ErrRebalanceInProgress:
    case ErrOffsetsLoadInProgress:
        if (retries <= 0)
        {
            co_return syncGroupResponse->Err;
        }
        co_return co_await RetryNewSession(ctx, topics, handler, retries, true, session);
    case ErrFencedInstancedId:
        Logger::Printf("JoinGroup failed: group instance id %s has been fenced\n", groupInstanceId);
        co_return syncGroupResponse->Err;
    default:
        co_return syncGroupResponse->Err;
    }

    std::map<std::string, std::vector<int32_t>> claims;
    if (!syncGroupResponse->MemberAssignment.empty())
    {
        std::shared_ptr<ConsumerGroupMemberAssignment> assignment;
        err = syncGroupResponse->GetMemberAssignment(assignment);
        if (err != 0)
        {
            co_return err;
        }
        claims = assignment->Topics;

        if (assignment->UserData.size() > 0)
        {
            userData = std::move(assignment->UserData);
        }
        else
        {
            userData = config_->Consumer.Group.Member.UserData;
        }

        for (auto &kv : claims)
        {
            std::sort(kv.second.begin(), kv.second.end());
        }
    }

    err = co_await NewConsumerGroupSession(ctx, shared_from_this(), claims, response->MemberId, response->GenerationId, handler, session);

    if (err != 0)
    {

        co_return err;
    }

    if (response->LeaderId == response->MemberId)
    {
        task_ << LoopCheckPartitionNumbers(allSubscribedTopicPartitions, allSubscribedTopics, session);
    }

    co_return err;
}

coev::awaitable<int> ConsumerGroup::JoinGroup(std::shared_ptr<Broker> coordinator, const std::vector<std::string> &topics, std::shared_ptr<JoinGroupResponse> &response)
{
    auto request = std::make_shared<JoinGroupRequest>();
    request->GroupId = groupId;
    request->MemberId = memberId;
    request->SessionTimeout = static_cast<int32_t>(config_->Consumer.Group.Session.Timeout.count() / 1000000); // assuming time.Millisecond = 1e6 ns
    request->ProtocolType = "consumer";

    if (config_->Version.IsAtLeast(V0_10_1_0))
    {
        request->Version = 1;
        request->RebalanceTimeout = static_cast<int32_t>(config_->Consumer.Group.Rebalance.Timeout.count() / 1000000);
    }
    if (config_->Version.IsAtLeast(V0_11_0_0))
    {
        request->Version = 2;
    }
    if (config_->Version.IsAtLeast(V2_0_0_0))
    {
        request->Version = 3;
    }
    if (config_->Version.IsAtLeast(V2_2_0_0))
    {
        request->Version = 4;
    }
    if (config_->Version.IsAtLeast(V2_3_0_0))
    {
        request->Version = 5;
        request->GroupInstanceId = groupInstanceId;
        if (config_->Version.IsAtLeast(V2_4_0_0))
        {
            request->Version = 6;
        }
    }

    auto meta = std::make_shared<ConsumerGroupMemberMetadata>();
    meta->Topics = topics;
    meta->UserData = userData;

    int err = 0;
    auto strategy = config_->Consumer.Group.Rebalance.Strategy;
    if (strategy != nullptr)
    {
        err = request->AddGroupProtocolMetadata(strategy->Name(), meta);
        if (err != 0)
        {
            co_return err;
        }
    }
    else
    {
        for (const auto &s : config_->Consumer.Group.Rebalance.GroupStrategies)
        {
            err = request->AddGroupProtocolMetadata(s->Name(), meta);
            if (err != 0)
            {
                co_return err;
            }
        }
    }

    err = co_await coordinator->JoinGroup(request, response);
    if (err != 0)
    {
        co_return err;
    }
    co_return 0;
}

std::shared_ptr<BalanceStrategy> ConsumerGroup::findStrategy(const std::string &name, const std::vector<std::shared_ptr<BalanceStrategy>> &groupStrategies, bool &ok)
{
    for (const auto &strategy : groupStrategies)
    {
        if (strategy->Name() == name)
        {
            ok = true;
            return strategy;
        }
    }
    ok = false;
    return nullptr;
}

coev::awaitable<int> ConsumerGroup::SyncGroup(std::shared_ptr<Broker> coordinator, std::map<std::string, ConsumerGroupMemberMetadata> members,
                                                     const BalanceStrategyPlan &plan, int32_t generationID,
                                                     std::shared_ptr<BalanceStrategy> strategy, std::shared_ptr<SyncGroupResponse> &response)
{
    auto req = std::make_shared<SyncGroupRequest>();
    req->GroupId = groupId;
    req->MemberId = memberId;
    req->GenerationId = generationID;

    if (config_->Version.IsAtLeast(V1_1_0_0))
    { // V0_11_0_0
        req->Version = 1;
    }
    if (config_->Version.IsAtLeast(V2_0_0_0))
    { // V2_0_0_0
        req->Version = 2;
    }
    if (config_->Version.IsAtLeast(V2_3_0_0))
    { // V2_3_0_0
        req->Version = 3;
        req->GroupInstanceId = groupInstanceId;
        if (config_->Version.IsAtLeast(V2_4_0_0))
        { // V2_4_0_0
            req->Version = 4;
        }
    }

    int err = 0;
    for (const auto &kv : plan)
    {
        const std::string &memberID = kv.first;
        const auto &topics = kv.second;

        auto assignment = std::make_shared<ConsumerGroupMemberAssignment>();
        assignment->Topics = topics;

        std::string userDataBytes;
        err = strategy->AssignmentData(memberID, topics, generationID, userDataBytes);
        if (err != 0)
        {
            co_return err;
        }
        assignment->UserData = userDataBytes;

        err = req->AddGroupAssignmentMember(memberID, assignment);
        if (err != 0)
        {
            co_return err;
        }
        members.erase(memberID);
    }

    for (const auto &kv : members)
    {
        const std::string &memberID = kv.first;
        err = req->AddGroupAssignmentMember(memberID, std::make_shared<ConsumerGroupMemberAssignment>());
        if (err != 0)
        {
            co_return err;
        }
    }

    err = co_await coordinator->SyncGroup(req, response);
    if (err != 0)
    {
        co_return err;
    }
    co_return 0;
}

coev::awaitable<int> ConsumerGroup::Heartbeat(std::shared_ptr<Broker> coordinator, const std::string &memberID, int32_t generationID,
                                                     std::shared_ptr<HeartbeatResponse> &response)
{
    auto req = std::make_shared<HeartbeatRequest>();
    req->GroupId = groupId;
    req->MemberId = memberID;
    req->GenerationId = generationID;

    if (config_->Version.IsAtLeast(V0_11_0_0))
    {
        req->Version = 1;
    }
    if (config_->Version.IsAtLeast(V2_0_0_0))
    {
        req->Version = 2;
    }
    if (config_->Version.IsAtLeast(V2_3_0_0))
    {
        req->Version = 3;
        req->GroupInstanceId = groupInstanceId;
        if (config_->Version.IsAtLeast(V2_4_0_0))
        {
            req->Version = 4;
        }
    }

    co_return co_await coordinator->Heartbeat(req, response);
}

coev::awaitable<int> ConsumerGroup::Balance(std::shared_ptr<BalanceStrategy> strategy,
                                            const std::map<std::string, ConsumerGroupMemberMetadata> &members, std::map<std::string, std::vector<int32_t>> &topicPartitions,
                                            std::vector<std::string> &allSubscribedTopics, BalanceStrategyPlan &plan)
{
    topicPartitions.clear();
    for (auto &kv : members)
    {
        auto &meta = kv.second;
        for (auto &topic : meta.Topics)
        {
            topicPartitions[topic] = std::vector<int32_t>();
        }
    }

    allSubscribedTopics.clear();
    allSubscribedTopics.reserve(topicPartitions.size());
    for (const auto &kv : topicPartitions)
    {
        allSubscribedTopics.push_back(kv.first);
    }

    int err = co_await client_->RefreshMetadata(allSubscribedTopics);
    if (err != 0)
    {
        co_return err;
    }

    for (auto &kv : topicPartitions)
    {
        const std::string &topic = kv.first;
        std::vector<int32_t> partitions;
        err = co_await client_->Partitions(topic, partitions);
        if (err != 0)
        {
            co_return err;
        }
        kv.second = partitions;
    }

    co_return strategy->Plan(members, topicPartitions, plan);
}

coev::awaitable<int> ConsumerGroup::Leave()
{
    std::lock_guard<std::mutex> lock_guard(lock);
    if (memberId.empty())
    {
        co_return 0;
    }

    auto err = co_await client_->GetCoordinator(groupId, coordinator);
    if (err != ErrNoError)
    {
        co_return -1;
    }

    if (groupInstanceId.size() > 0)
    {
        memberId.clear();
        co_return 0;
    }

    auto req = std::make_shared<LeaveGroupRequest>();
    req->GroupId = groupId;
    req->MemberId = memberId;

    if (config_->Version.IsAtLeast(V0_11_0_0))
    {
        req->Version = 1;
    }
    if (config_->Version.IsAtLeast(V2_0_0_0))
    {
        req->Version = 2;
    }
    if (config_->Version.IsAtLeast(V2_4_0_0))
    {
        req->Version = 4;
        req->Members.push_back(MemberIdentity{memberId});
    }

    std::shared_ptr<LeaveGroupResponse> resp;
    err = co_await coordinator->LeaveGroup(req, resp);
    if (err != ErrNoError)
    {
        co_await coordinator->Close();
        co_return err;
    }

    memberId = "";

    switch (resp->Err)
    {
    case ErrRebalanceInProgress:
    case ErrUnknownMemberId:
    case ErrNoError:
        co_return ErrNoError;
    default:
        co_return resp->Err;
    }
}

void ConsumerGroup::HandleError(std::shared_ptr<ConsumerError> err, const std::string &topic, int32_t partition)
{
    if (!err && !topic.empty() && partition > -1)
    {
        err = std::make_shared<ConsumerError>();
        err->Topic = topic;
        err->Partition = partition;
        err->Err = ErrNoError;
    }

    if (!config_->Consumer.Return.Errors)
    {
        Logger::Println(KErrorToString(err->Err));
        return;
    }

    std::shared_lock<std::shared_mutex> lock(errorsLock);
    bool _closed = false;
    if (closed.try_get(_closed))
    {
        return;
    }

    errors.set(err);
}

coev::awaitable<void> ConsumerGroup::LoopCheckPartitionNumbers(
    const std::map<std::string, std::vector<int32_t>> &allSubscribedTopicPartitions,
    const std::vector<std::string> &topics, std::shared_ptr<ConsumerGroupSession> &session)
{
    if (config_->Metadata.RefreshFrequency == std::chrono::milliseconds(0))
    {
        co_return;
    }

    session->cancel();

    std::map<std::string, int> oldTopicToPartitionNum;
    for (auto &kv : allSubscribedTopicPartitions)
    {
        oldTopicToPartitionNum[kv.first] = static_cast<int>(kv.second.size());
    }

    while (true)
    {
        std::map<std::string, int> newTopicToPartitionNum;
        int err = co_await TopicToPartitionNumbers(topics, newTopicToPartitionNum);
        if (err != 0)
        {
            co_return;
        }

        bool changed = false;
        for (auto &kv : oldTopicToPartitionNum)
        {
            const std::string &topic = kv.first;
            int oldNum = kv.second;
            int newNum = newTopicToPartitionNum[topic];
            if (newNum != oldNum)
            {
                Logger::Printf(
                    "consumergroup/%s loop check partition number goroutine find partitions in topics %s changed from %d to %d\n",
                    groupId.c_str(), topics.empty() ? "" : topics[0].c_str(), oldNum, newNum);
                co_return;
            }
        }

        coev::co_task task_;
        task_ << [this]() -> coev::awaitable<void>
        {
            co_await sleep_for(config_->Metadata.RefreshFrequency);
        }();
        task_ << [&session]() -> coev::awaitable<void>
        {
            co_await session->context->get().get();
        }();
        task_ << closed.get();

        co_await task_.wait();
    }
}

coev::awaitable<int> ConsumerGroup::TopicToPartitionNumbers(const std::vector<std::string> &topics, std::map<std::string, int> &topicToPartitionNum)
{
    topicToPartitionNum.clear();

    for (auto &topic : topics)
    {
        std::vector<int32_t> partitionNum;
        int err = co_await client_->Partitions(topic, partitionNum);
        if (err != 0)
        {
            Logger::Printf(
                "consumergroup/%s topic %s get partition number failed due to '%d'\n",
                groupId.c_str(), topic.c_str(), err);
            co_return err;
        }
        topicToPartitionNum[topic] = static_cast<int>(partitionNum.size());
    }
    co_return 0;
}

coev::awaitable<int> NewConsumerGroup(const std::vector<std::string> &addrs, const std::string &group_id, std::shared_ptr<Config> config, std::shared_ptr<IConsumerGroup> &consumer_group)
{
    std::shared_ptr<Client> client;
    auto err = co_await NewClient(addrs, config, client);
    if (err != 0)
    {
        co_return err;
    }

    try
    {
        std::shared_ptr<IConsumer> consumer;
        auto err = NewConsumer(client, consumer);
        if (err != 0)
        {
            co_return err;
        }
        consumer_group = std::make_shared<ConsumerGroup>(client, consumer, config, group_id);
    }
    catch (...)
    {
        co_return client->Close();
    }
    co_return ErrNoError;
}

coev::awaitable<int> NewConsumerGroupSession(std::shared_ptr<Context> context, std::shared_ptr<ConsumerGroup> parent, std::map<std::string, std::vector<int32_t>> &claims, const std::string &memberID, int32_t generationID, std::shared_ptr<ConsumerGroupHandler> &handler, std::shared_ptr<ConsumerGroupSession> &session)
{

    std::function<void()> cancel;
    std::shared_ptr<OffsetManager> offsets;
    auto err = co_await NewOffsetManagerFromClient(parent->groupId, "", -1, parent->client_, cancel, offsets);
    if (err != ErrNoError)
    {
        co_return err;
    }

    handler = std::make_shared<ConsumerGroupHandler>();
    session = std::make_shared<ConsumerGroupSession>(parent, "", -1, handler, offsets, claims, context, cancel);

    session->task_ << session->HeartbeatLoop_();

    for (auto &claim : claims)
    {
        auto &topic = claim.first;
        auto &partitions = claim.second;
        for (auto partition : partitions)
        {
            auto pom = offsets->ManagePartition(topic, partition);
            if (err != ErrNoError)
            {
                session->Release_(false);
                co_return ErrNoError;
            }

            session->task_ << [session, pom, topic, partition]() -> coev::awaitable<void>
            {
                while (true)
                {
                    auto err = co_await pom->Errors().get();
                    if (err->Err != ErrNoError)
                        break;
                    auto e = std::make_shared<ConsumerError>(topic, partition, err->Err);
                    session->parent->HandleError(e, topic, partition);
                }
                co_return;
            }();
        }
    }

    err = handler->Setup(session);
    if (err != ErrNoError)
    {
        session->Release_(true);
        co_return 0;
    }

    for (auto &claim : claims)
    {
        const std::string &topic = claim.first;
        const std::vector<int32_t> &partitions = claim.second;
        for (int32_t partition : partitions)
        {
            session->waitgroup_.add();
            co_start << [session, topic, partition]() -> coev::awaitable<void>
            {
                if (session->parent->client_->PartitionNotReadable(topic, partition))
                {
                    while (session->parent->client_->PartitionNotReadable(topic, partition))
                    {
                        bool isClosed;
                        if (session->parent->closed.try_get(isClosed))
                        {
                            co_return;
                        }
                        co_await sleep_for(std::chrono::seconds(5));
                    }
                }
            }();
            co_await session->Consume_(topic, partition);
        }
    }

    co_return 0;
}