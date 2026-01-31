#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include "version.h"
#include "errors.h"
#include "consumer.h"
#include "sleep_for.h"
#include "offset_manager.h"
#include "partition_offset_manager.h"
#include "consumer_group.h"
#include "consumer_group_session.h"
#include "consumer_group_claim.h"
#include "consumer_group_handler.h"

ConsumerGroup::ConsumerGroup(std::shared_ptr<Client> client, std::shared_ptr<IConsumer> consumer, std::shared_ptr<Config> config, const std::string &group_)
    : m_client(client), m_config(config), m_consumer(consumer), m_group_id(group_)
{

    m_user_data = m_config->Consumer.Group.Member.UserData;

    if (!m_config->Version.IsAtLeast(V0_10_2_0))
    {
        throw std::runtime_error("consumer groups require Version to be >= V0_10_2_0");
    }

    if (m_config->Consumer.Group.InstanceID != "" && m_config->Version.IsAtLeast(V2_3_0_0))
    {
        m_group_instance_id = m_config->Consumer.Group.InstanceID;
    }
}

coev::awaitable<int> ConsumerGroup::Consume(std::shared_ptr<Context> &ctx, const std::vector<std::string> &topics, std::shared_ptr<ConsumerGroupHandler> handler)
{
    bool isClosed;
    co_await m_closed.get(isClosed);
    if (isClosed)
    {
        co_return ErrorGroupClosed;
    }

    if (topics.empty())
    {
        co_return ErrTopicsProvided;
    }

    if (int err = co_await m_client->RefreshMetadata(topics); err != 0)
    {
        co_return err;
    }
    std::shared_ptr<ConsumerGroupSession> session;
    auto err = co_await NewSession(ctx, topics, handler, m_config->Consumer.Group.Rebalance.Retry.Max, session);
    if (err != ErrNoError)
    {
        co_return err;
    }

    co_return co_await session->_Release(true);
}

coev::co_channel<std::shared_ptr<ConsumerError>> &ConsumerGroup::Errors()
{
    return m_errors;
}

coev::awaitable<int> ConsumerGroup::Close()
{
    bool isClosed;
    co_await m_closed.get(isClosed);
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
    while (m_errors.try_get(cErr))
    {
    }

    if (err = m_client->Close(); err != 0)
    {
        co_return err;
    }

    co_return err;
}

void ConsumerGroup::Pause(const std::map<std::string, std::vector<int32_t>> &partitions)
{
    m_consumer->Pause(partitions);
}

void ConsumerGroup::Resume(const std::map<std::string, std::vector<int32_t>> &partitions)
{
    m_consumer->Resume(partitions);
}

void ConsumerGroup::PauseAll()
{
    m_consumer->PauseAll();
}

void ConsumerGroup::ResumeAll()
{
    m_consumer->ResumeAll();
}

coev::awaitable<int> ConsumerGroup::RetryNewSession(std::shared_ptr<Context> &ctx, const std::vector<std::string> &topics, std::shared_ptr<ConsumerGroupHandler> handler, int retries, bool refreshCoordinator, std::shared_ptr<ConsumerGroupSession> &out)
{
    co_task _task;
    auto __get = _task << [](auto ctx) -> coev::awaitable<void>
    {
        bool dummy;
        co_await ctx->ch.get(dummy);
        co_return;
    }(ctx);
    auto __close = _task << [](auto _this) -> coev::awaitable<void>
    {
        bool dummy;
        co_await _this->m_closed.get(dummy);
        co_return;
    }(shared_from_this());
    auto __config = _task << [](auto _this) -> coev::awaitable<void>
    {
        co_await sleep_for(_this->m_config->Consumer.Group.Rebalance.Retry.Backoff);
        co_return;
    }(shared_from_this());
    auto select_result = co_await _task.wait();
    if (select_result == __get)
    {
        co_return 0;
    }
    else if (select_result == __close)
    {
        co_return ErrClosedClient;
    }

    if (refreshCoordinator)
    {
        auto err = co_await m_client->RefreshCoordinator(m_group_id);
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

    int err = co_await m_client->GetCoordinator(m_group_id, m_coordinator);
    if (err != 0)
    {
        if (retries <= 0)
        {
            co_return err;
        }
        co_return co_await RetryNewSession(ctx, topics, handler, retries, true, session);
    }

    JoinGroupResponse response;
    err = co_await JoinGroup(m_coordinator, topics, response);
    if (err != 0)
    {
        m_coordinator->Close();
        co_return err;
    }

    switch (response.m_err)
    {
    case ErrNoError:
        m_member_id = response.m_member_id;
        break;
    case ErrUnknownMemberId:
    case ErrIllegalGeneration:
        m_member_id = "";
        co_return co_await NewSession(ctx, topics, handler, retries, session);
    case ErrNotCoordinatorForConsumer:
    case ErrRebalanceInProgress:
    case ErrOffsetsLoadInProgress:
        if (retries <= 0)
        {
            co_return response.m_err;
        }
        co_return co_await RetryNewSession(ctx, topics, handler, retries, true, session);
    case ErrMemberIdRequired:
        m_member_id = response.m_member_id;
        co_return co_await NewSession(ctx, topics, handler, retries, session);
    case ErrFencedInstancedId:
        LOG_CORE("JoinGroup failed: group instance id %s has been fenced", m_group_instance_id.data());
        co_return response.m_err;
    default:
        co_return response.m_err;
    }

    std::shared_ptr<BalanceStrategy> strategy = m_config->Consumer.Group.Rebalance.Strategy;
    bool ok = false;
    if (strategy == nullptr)
    {
        strategy = FindStrategy(response.m_group_protocol, m_config->Consumer.Group.Rebalance.GroupStrategies, ok);
        if (!ok)
        {
            co_return ErrStrategyNotFound;
        }
    }

    std::map<std::string, ConsumerGroupMemberMetadata> members;
    std::map<std::string, std::vector<int32_t>> allSubscribedTopicPartitions;
    std::vector<std::string> allSubscribedTopics;
    BalanceStrategyPlan plan;

    if (response.m_leader_id == response.m_member_id)
    {
        err = response.get_members(members);
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

    SyncGroupResponse syncGroupResponse;
    err = co_await SyncGroup(m_coordinator, members, plan, response.m_generation_id, strategy, syncGroupResponse);
    if (err != 0)
    {
        m_coordinator->Close();
        co_return err;
    }

    switch (syncGroupResponse.m_err)
    {
    case ErrNoError:
        break;
    case ErrUnknownMemberId:
    case ErrIllegalGeneration:
        m_member_id = "";
        co_return co_await NewSession(ctx, topics, handler, retries, session);
    case ErrNotCoordinatorForConsumer:
    case ErrRebalanceInProgress:
    case ErrOffsetsLoadInProgress:
        if (retries <= 0)
        {
            co_return syncGroupResponse.m_err;
        }
        co_return co_await RetryNewSession(ctx, topics, handler, retries, true, session);
    case ErrFencedInstancedId:
        LOG_CORE("JoinGroup failed: group instance id %s has been fenced", m_group_instance_id.data());
        co_return syncGroupResponse.m_err;
    default:
        co_return syncGroupResponse.m_err;
    }

    std::map<std::string, std::vector<int32_t>> claims;
    if (!syncGroupResponse.m_member_assignment.empty())
    {
        std::shared_ptr<ConsumerGroupMemberAssignment> assignment;
        err = syncGroupResponse.GetMemberAssignment(assignment);
        if (err != 0)
        {
            co_return err;
        }
        claims = assignment->Topics;

        if (assignment->UserData.size() > 0)
        {
            m_user_data = std::move(assignment->UserData);
        }
        else
        {
            m_user_data = m_config->Consumer.Group.Member.UserData;
        }

        for (auto &kv : claims)
        {
            std::sort(kv.second.begin(), kv.second.end());
        }
    }

    err = co_await NewConsumerGroupSession(ctx, shared_from_this(), claims, response.m_member_id, response.m_generation_id, handler, session);

    if (err != 0)
    {
        co_return err;
    }

    if (response.m_leader_id == response.m_member_id)
    {
        m_task << LoopCheckPartitionNumbers(allSubscribedTopicPartitions, allSubscribedTopics, session);
    }

    co_return ErrNoError;
}

coev::awaitable<int> ConsumerGroup::JoinGroup(std::shared_ptr<Broker> coordinator, const std::vector<std::string> &topics, JoinGroupResponse &response)
{
    auto request = std::make_shared<JoinGroupRequest>();
    request->m_group_id = m_group_id;
    request->m_member_id = m_member_id;
    request->m_session_timeout = static_cast<int32_t>(m_config->Consumer.Group.Session.Timeout.count() / 1000000); // assuming time.Millisecond = 1e6 ns
    request->m_protocol_type = "consumer";

    if (m_config->Version.IsAtLeast(V0_10_1_0))
    {
        request->m_version = 1;
        request->m_rebalance_timeout = static_cast<int32_t>(m_config->Consumer.Group.Rebalance.Timeout.count() / 1000000);
    }
    if (m_config->Version.IsAtLeast(V0_11_0_0))
    {
        request->m_version = 2;
    }
    if (m_config->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 3;
    }
    if (m_config->Version.IsAtLeast(V2_2_0_0))
    {
        request->m_version = 4;
    }
    if (m_config->Version.IsAtLeast(V2_3_0_0))
    {
        request->m_version = 5;
        request->m_group_instance_id = m_group_instance_id;
        if (m_config->Version.IsAtLeast(V2_4_0_0))
        {
            request->m_version = 6;
        }
    }

    auto meta = std::make_shared<ConsumerGroupMemberMetadata>();
    meta->m_topics = topics;
    meta->m_user_data = m_user_data;

    int err = 0;
    auto strategy = m_config->Consumer.Group.Rebalance.Strategy;
    if (strategy != nullptr)
    {
        err = request->add_group_protocol_metadata(strategy->Name(), meta);
        if (err != 0)
        {
            co_return err;
        }
    }
    else
    {
        for (const auto &s : m_config->Consumer.Group.Rebalance.GroupStrategies)
        {
            err = request->add_group_protocol_metadata(s->Name(), meta);
            if (err != 0)
            {
                co_return err;
            }
        }
    }

    ResponsePromise<JoinGroupResponse> promise;
    err = co_await coordinator->JoinGroup(request, promise);
    if (err != 0)
    {
        co_return err;
    }
    response = promise.m_response;
    co_return 0;
}

std::shared_ptr<BalanceStrategy> ConsumerGroup::FindStrategy(const std::string &name, const std::vector<std::shared_ptr<BalanceStrategy>> &groupStrategies, bool &ok)
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
                                              std::shared_ptr<BalanceStrategy> strategy, SyncGroupResponse &response)
{
    auto req = std::make_shared<SyncGroupRequest>();
    req->m_group_id = m_group_id;
    req->m_member_id = m_member_id;
    req->m_generation_id = generationID;

    if (m_config->Version.IsAtLeast(V1_1_0_0))
    {
        req->m_version = 1;
    }
    if (m_config->Version.IsAtLeast(V2_0_0_0))
    {
        req->m_version = 2;
    }
    if (m_config->Version.IsAtLeast(V2_3_0_0))
    {
        req->m_version = 3;
        req->m_group_instance_id = m_group_instance_id;
        if (m_config->Version.IsAtLeast(V2_4_0_0))
        {
            req->m_version = 4;
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

    ResponsePromise<SyncGroupResponse> sync_promise;
    err = co_await coordinator->SyncGroup(req, sync_promise);
    if (err != 0)
    {
        co_return err;
    }
    response = sync_promise.m_response;
    co_return 0;
}

coev::awaitable<int> ConsumerGroup::Heartbeat(std::shared_ptr<Broker> coordinator, const std::string &memberID, int32_t generationID,
                                              std::shared_ptr<HeartbeatResponse> &response)
{
    auto req = std::make_shared<HeartbeatRequest>();
    req->m_group_id = m_group_id;
    req->m_member_id = memberID;
    req->m_generation_id = generationID;

    if (m_config->Version.IsAtLeast(V0_11_0_0))
    {
        req->m_version = 1;
    }
    if (m_config->Version.IsAtLeast(V2_0_0_0))
    {
        req->m_version = 2;
    }
    if (m_config->Version.IsAtLeast(V2_3_0_0))
    {
        req->m_version = 3;
        req->m_group_instance_id = m_group_instance_id;
        if (m_config->Version.IsAtLeast(V2_4_0_0))
        {
            req->m_version = 4;
        }
    }

    ResponsePromise<HeartbeatResponse> promise;
    int err = co_await coordinator->Heartbeat(req, promise);
    if (err == 0)
    {
        response = std::make_shared<HeartbeatResponse>(promise.m_response);
    }
    co_return err;
}

coev::awaitable<int> ConsumerGroup::Balance(std::shared_ptr<BalanceStrategy> strategy,
                                            const std::map<std::string, ConsumerGroupMemberMetadata> &members, std::map<std::string, std::vector<int32_t>> &topicPartitions,
                                            std::vector<std::string> &allSubscribedTopics, BalanceStrategyPlan &plan)
{
    topicPartitions.clear();
    for (auto &kv : members)
    {
        auto &meta = kv.second;
        for (auto &topic : meta.m_topics)
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

    int err = co_await m_client->RefreshMetadata(allSubscribedTopics);
    if (err != 0)
    {
        co_return err;
    }

    for (auto &kv : topicPartitions)
    {
        const std::string &topic = kv.first;
        std::vector<int32_t> partitions;
        err = co_await m_client->Partitions(topic, partitions);
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
    if (m_member_id.empty())
    {
        co_return 0;
    }

    auto err = co_await m_client->GetCoordinator(m_group_id, m_coordinator);
    if (err != ErrNoError)
    {
        co_return err;
    }

    auto req = std::make_shared<LeaveGroupRequest>();
    req->m_group_id = m_group_id;
    req->m_member_id = m_member_id;

    if (m_config->Version.IsAtLeast(V0_11_0_0))
    {
        req->m_version = 1;
    }
    if (m_config->Version.IsAtLeast(V2_0_0_0))
    {
        req->m_version = 2;
    }
    if (m_config->Version.IsAtLeast(V2_3_0_0))
    {
        req->m_version = 3;
    }
    if (m_config->Version.IsAtLeast(V2_4_0_0))
    {
        req->m_version = 4;
        req->m_members.push_back(MemberIdentity{m_member_id, m_group_instance_id});
    }

    ResponsePromise<LeaveGroupResponse> promise;
    err = co_await m_coordinator->LeaveGroup(req, promise);
    if (err != ErrNoError)
    {
        m_coordinator->Close();
        co_return err;
    }

    m_member_id = "";

    switch (promise.m_response.m_err)
    {
    case ErrRebalanceInProgress:
    case ErrUnknownMemberId:
    case ErrNoError:
        co_return ErrNoError;
    default:
        co_return promise.m_response.m_err;
    }
}

void ConsumerGroup::HandleError(std::shared_ptr<ConsumerError> err, const std::string &topic, int32_t partition)
{
    if (!err && !topic.empty() && partition > -1)
    {
        err = std::make_shared<ConsumerError>();
        err->m_topic = topic;
        err->m_partition = partition;
        err->m_err = ErrNoError;
    }

    if (!m_config->Consumer.Return.Errors)
    {
        LOG_CORE("%s", KErrorToString(err->m_err));
        return;
    }

    bool _closed = false;
    if (m_closed.try_get(_closed))
    {
        return;
    }

    m_errors.set(err);
}

coev::awaitable<void> ConsumerGroup::LoopCheckPartitionNumbers(
    const std::map<std::string, std::vector<int32_t>> &allSubscribedTopicPartitions,
    const std::vector<std::string> &topics, std::shared_ptr<ConsumerGroupSession> &session)
{
    if (m_config->Metadata.RefreshFrequency == std::chrono::milliseconds(0))
    {
        co_return;
    }

    session->m_cancel();

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
                LOG_CORE("loop check partition number goroutine find partitions in topics %s changed from %d to %d",
                         topics.empty() ? "" : topics[0].c_str(), oldNum, newNum);
                co_return;
            }
        }

        coev::co_task task_;
        task_ << [this]() -> coev::awaitable<void>
        {
            co_await sleep_for(m_config->Metadata.RefreshFrequency);
        }();
        m_task << [&session]() -> coev::awaitable<void>
        {
            bool dummy;
            co_await session->m_context->get().get(dummy);
            co_return;
        }();
        m_task << [this]() -> coev::awaitable<void>
        {
            bool dummy;
            co_await m_closed.get(dummy);
            co_return;
        }();

        co_await m_task.wait();
    }
}

coev::awaitable<int> ConsumerGroup::TopicToPartitionNumbers(const std::vector<std::string> &topics, std::map<std::string, int> &topicToPartitionNum)
{
    topicToPartitionNum.clear();

    for (auto &topic : topics)
    {
        std::vector<int32_t> partitionNum;
        int err = co_await m_client->Partitions(topic, partitionNum);
        if (err != 0)
        {
            LOG_CORE("topic %s get partition number failed due to '%d'",
                     topic.c_str(), err);
            co_return err;
        }
        topicToPartitionNum[topic] = static_cast<int>(partitionNum.size());
    }
    co_return 0;
}

coev::awaitable<int> NewConsumerGroup(const std::vector<std::string> &addrs, const std::string &group_id, std::shared_ptr<Config> config, std::shared_ptr<ConsumerGroup> &consumer_group)
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
    auto err = co_await NewOffsetManagerFromClient(parent->m_group_id, "", -1, parent->m_client, cancel, offsets);
    if (err != ErrNoError)
    {
        co_return err;
    }

    session = std::make_shared<ConsumerGroupSession>(parent, memberID, generationID, handler, offsets, claims, context, cancel);

    for (auto &claim : claims)
    {
        auto &topic = claim.first;
        auto &partitions = claim.second;
        for (auto partition : partitions)
        {
            auto pom = offsets->ManagePartition(topic, partition);
            if (err != ErrNoError)
            {
                session->_Release(false);
                co_return ErrNoError;
            }

            session->m_task << [session, pom, topic, partition]() -> coev::awaitable<void>
            {
                while (true)
                {
                    std::shared_ptr<ConsumerError> err;
                    co_await pom->Errors(err);
                    if (!err || err->m_err != ErrNoError)
                        break;
                    auto e = std::make_shared<ConsumerError>(topic, partition, err->m_err);
                    session->m_parent->HandleError(e, topic, partition);
                }
                co_return;
            }();
            session->m_task << session->HanderError(pom, topic, partition);
        }
    }

    err = handler->Setup(session);
    if (err != ErrNoError)
    {
        session->_Release(true);
        co_return 0;
    }

    for (auto &claim : claims)
    {
        const std::string &topic = claim.first;
        const std::vector<int32_t> &partitions = claim.second;
        for (int32_t partition : partitions)
        {
            session->m_waitgroup.add();
            co_start << [session, topic, partition]() -> coev::awaitable<void>
            {
                if (session->m_parent->m_client->PartitionNotReadable(topic, partition))
                {
                    while (session->m_parent->m_client->PartitionNotReadable(topic, partition))
                    {
                        bool isClosed;
                        if (session->m_parent->m_closed.try_get(isClosed))
                        {
                            co_return;
                        }
                        co_await sleep_for(std::chrono::seconds(5));
                    }
                }
            }();
            co_await session->_Consume(topic, partition);
        }
    }

    co_return 0;
}