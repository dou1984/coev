#include "broker_consumer.h"
#include "partition_consumer.h"
#include "response_header.h"
#include "sleep_for.h"

auto partitionConsumersBatchTimeout = std::chrono::milliseconds(100);

coev::awaitable<void> BrokerConsumer::SubscriptionManager()
{
    while (true)
    {
        std::vector<std::shared_ptr<PartitionConsumer>> partitionConsumers;

        auto pc = co_await m_input.get();
        if (!pc)
            co_return;
        partitionConsumers.emplace_back(pc);

        auto timerEnd = std::chrono::steady_clock::now() + partitionConsumersBatchTimeout;
        while (std::chrono::steady_clock::now() < timerEnd)
        {

            if (pc = co_await m_input.get())
            {
                if (pc)
                {
                    partitionConsumers.emplace_back(pc);
                }
            }
            else
            {
                co_await sleep_for(std::chrono::milliseconds(1));
            }
        }

        m_new_subscriptions = std::move(partitionConsumers);
    }
}

coev::awaitable<void> BrokerConsumer::SubscriptionConsumer()
{

    while (true)
    {
        std::vector<std::shared_ptr<PartitionConsumer>> sub = co_await m_new_subscriptions.get();
        UpdateSubscriptions(sub);

        if (m_subscriptions.empty())
        {
            co_await sleep_for(partitionConsumersBatchTimeout);
            continue;
        }

        std::shared_ptr<FetchResponse> response;
        int err = co_await FetchNewMessages(response);
        if (err != 0)
        {
            Abort(err);
            co_return;
        }

        if (!response)
        {
            co_await sleep_for(partitionConsumersBatchTimeout);
            co_return;
        }

        for (auto &child : m_subscriptions)
        {
            auto it1 = response->m_blocks.find(child.first->m_topic);
            if (it1 == response->m_blocks.end())
            {
                continue;
            }
            auto it2 = it1->second.find(child.first->m_partition);
            if (it2 == it1->second.end())
            {
                continue;
            }
            child.first->m_feeder.set(response);
        }

        co_await HandleResponses();
    }
}
void BrokerConsumer::UpdateSubscriptions(const std::vector<std::shared_ptr<PartitionConsumer>> &m_new_subscriptions)
{
    for (auto &child : m_new_subscriptions)
    {
        m_subscriptions[child] = true;
    }

    for (auto it = m_subscriptions.begin(); it != m_subscriptions.end();)
    {
        bool found = false;
        auto child = it->first;
        if (child->m_dying.try_get(found))
        {
            child->m_trigger.set(true);
            it = m_subscriptions.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

coev::awaitable<void> BrokerConsumer::HandleResponses()
{
    for (auto it = m_subscriptions.begin(); it != m_subscriptions.end();)
    {
        auto child = it->first;
        auto result = child->m_response_result;
        child->m_response_result = ErrNoError;

        if (!result)
        {
            std::shared_ptr<Broker> preferredBroker;
            int _;
            auto err_code = co_await child->PreferredBroker(preferredBroker, _);
            if (err_code == 0 && m_broker->ID() != preferredBroker->ID())
            {
                child->m_trigger.set(true);
                it = m_subscriptions.erase(it);
                continue;
            }
            ++it;
            continue;
        }

        child->m_preferred_read_replica = invalidPreferredReplicaID;

        if (result == ErrTimedOut)
        {
            it = m_subscriptions.erase(it);
        }
        else if (result == ErrOffsetOutOfRange)
        {
            child->SendError(result);
            child->m_trigger.set(true);
            it = m_subscriptions.erase(it);
        }
        else if (result == ErrUnknownTopicOrPartition || result == ErrNotLeaderForPartition ||
                 result == ErrLeaderNotAvailable || result == ErrReplicaNotAvailable ||
                 result == ErrFencedLeaderEpoch || result == ErrUnknownLeaderEpoch)
        {
            child->m_trigger.set(true);
            it = m_subscriptions.erase(it);
        }
        else
        {
            child->SendError(result);
            child->m_trigger.set(true);
            it = m_subscriptions.erase(it);
        }
    }
}

coev::awaitable<void> BrokerConsumer::Abort(int err)
{
    m_consumer->AbandonBrokerConsumer(shared_from_this());
    m_broker->Close();

    for (auto &child : m_subscriptions)
    {
        child.first->SendError(err);
        child.first->m_trigger.set(true);
    }

    while (true)
    {
        auto _m_subscriptions = co_await m_new_subscriptions.get();
        if (_m_subscriptions.empty())
        {
            co_await sleep_for(partitionConsumersBatchTimeout);
            continue;
        }
        for (auto &child : _m_subscriptions)
        {
            child->SendError(err);
            child->m_trigger.set(true);
        }
    }
}

coev::awaitable<int> BrokerConsumer::FetchNewMessages(std::shared_ptr<FetchResponse> &response)
{
    auto request = std::make_shared<FetchRequest>();
    request->m_min_bytes = m_consumer->m_conf->Consumer.Fetch.Min;
    request->m_max_wait_time = static_cast<int32_t>(m_consumer->m_conf->Consumer.MaxWaitTime.count() / std::chrono::milliseconds(1).count());

    if (m_consumer->m_conf->Version.IsAtLeast(V0_9_0_0))
    {
        request->m_version = 1;
    }
    if (m_consumer->m_conf->Version.IsAtLeast(V0_10_0_0))
    {
        request->m_version = 2;
    }
    if (m_consumer->m_conf->Version.IsAtLeast(V0_10_1_0))
    {
        request->m_version = 3;
        request->m_max_bytes = MaxResponseSize;
    }
    if (m_consumer->m_conf->Version.IsAtLeast(V0_11_0_0))
    {
        request->m_version = 5;
        request->m_isolation = m_consumer->m_conf->Consumer.IsolationLevel_;
    }
    if (m_consumer->m_conf->Version.IsAtLeast(V1_0_0_0))
    {
        request->m_version = 6;
    }
    if (m_consumer->m_conf->Version.IsAtLeast(V1_1_0_0))
    {
        request->m_version = 7;
        request->m_session_id = 0;
        request->m_session_epoch = -1;
    }
    if (m_consumer->m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 8;
    }
    if (m_consumer->m_conf->Version.IsAtLeast(V2_1_0_0))
    {
        request->m_version = 10;
    }
    if (m_consumer->m_conf->Version.IsAtLeast(V2_3_0_0))
    {
        request->m_version = 11;
        request->m_rack_id = m_consumer->m_conf->RackId;
    }

    for (auto &child : m_subscriptions)
    {
        if (!child.first->IsPaused())
        {
            request->AddBlock(child.first->m_topic, child.first->m_partition, child.first->m_offset, child.first->m_fetch_size, child.first->m_leader_epoch);
        }
    }

    if (request->m_blocks.empty())
    {
        response = nullptr;
        co_return 0;
    }

    ResponsePromise<FetchResponse> local_response;
    int err = co_await m_broker->Fetch(*request, local_response);
    if (err == 0)
    {
        response = std::make_shared<FetchResponse>(local_response.m_response);
    }
    co_return err;
}

std::shared_ptr<BrokerConsumer> NewBrokerConsumer(std::shared_ptr<Consumer> c, std::shared_ptr<Broker> m_broker)
{
    auto bc = std::make_shared<BrokerConsumer>();
    bc->m_consumer = c;
    bc->m_broker = m_broker;
    bc->m_refs = 0;

    bc->m_task << bc->SubscriptionManager();
    bc->m_task << bc->SubscriptionConsumer();

    return bc;
}

int BrokerConsumer::Topics(std::vector<std::string> &out)
{
    return m_consumer->Topics(out);
}
coev::awaitable<int> BrokerConsumer::Partitions(const std::string &topic, std::vector<int32_t> &out)
{
    return m_consumer->Partitions(topic, out);
}
coev::awaitable<int> BrokerConsumer::Close()
{
    return m_consumer->Close();
}
std::map<std::string, std::map<int32_t, int64_t>> BrokerConsumer::HighWaterMarks()
{
    return m_consumer->HighWaterMarks();
}

void BrokerConsumer::Pause(const std::map<std::string, std::vector<int32_t>> &topicPartitions)
{
    m_consumer->Pause(topicPartitions);
}
void BrokerConsumer::Resume(const std::map<std::string, std::vector<int32_t>> &topicPartitions)
{
    m_consumer->Resume(topicPartitions);
}
void BrokerConsumer::PauseAll()
{
    m_consumer->PauseAll();
}
void BrokerConsumer::ResumeAll()
{
    m_consumer->ResumeAll();
}