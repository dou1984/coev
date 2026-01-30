#include "broker_consumer.h"
#include "partition_consumer.h"
#include "response_header.h"
#include "sleep_for.h"

auto PartitionConsumersBatchTimeout = std::chrono::milliseconds(100);

BrokerConsumer::BrokerConsumer()
{
    m_task << SubscriptionManager();
    m_task << SubscriptionConsumer();
}
coev::awaitable<void> BrokerConsumer::SubscriptionManager()
{
    LOG_CORE("BrokerConsumer::SubscriptionManager starting");
    while (true)
    {
        std::shared_ptr<PartitionConsumer> pc;
        co_await m_input.get(pc);
        if (pc == nullptr)
        {
            co_await sleep_for(std::chrono::milliseconds(10));
        }
        LOG_CORE("BrokerConsumer::SubscriptionManager received PartitionConsumer for topic %s, partition %d", pc->m_topic.c_str(), pc->m_partition);
        std::vector<std::shared_ptr<PartitionConsumer>> partition_consumers;
        partition_consumers.emplace_back(pc);

        auto timer_end = std::chrono::steady_clock::now() + PartitionConsumersBatchTimeout;
        while (std::chrono::steady_clock::now() < timer_end)
        {
            if (m_input.try_get(pc))
            {
                partition_consumers.emplace_back(pc);
            }
            else
            {
                co_await sleep_for(std::chrono::milliseconds(10));
            }
        }

        m_new_subscriptions = std::move(partition_consumers);
    }
}

coev::awaitable<void> BrokerConsumer::SubscriptionConsumer()
{
    while (true)
    {
        UpdateSubscriptions();
        LOG_CORE("BrokerConsumer::SubscriptionConsumer has %zu active subscriptions", m_subscriptions.size());
        if (m_subscriptions.empty())
        {
            LOG_CORE("BrokerConsumer::SubscriptionConsumer no active subscriptions, sleeping");
            co_await sleep_for(PartitionConsumersBatchTimeout);
            continue;
        }
        auto response = std::make_shared<FetchResponse>();
        int err = co_await FetchNewMessages(response);
        if (err != 0)
        {
            LOG_CORE("BrokerConsumer::SubscriptionConsumer FetchNewMessages failed with error %d, aborting", err);
            co_await Abort(err);
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

void BrokerConsumer::UpdateSubscriptions()
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
    LOG_CORE("starting with %zu subscriptions", m_subscriptions.size());
    for (auto it = m_subscriptions.begin(); it != m_subscriptions.end();)
    {
        auto child = it->first;
        auto result = child->m_response_result;
        child->m_response_result = ErrNoError;

        if (!result)
        {
            LOG_CORE("no result for partition consumer %s:%d, checking preferred broker", child->m_topic.c_str(), child->m_partition);
            std::shared_ptr<Broker> preferredBroker;
            int _;
            auto err_code = co_await child->PreferredBroker(preferredBroker, _);
            if (err_code == 0 && m_broker->ID() != preferredBroker->ID())
            {
                LOG_CORE("preferred broker changed for %s:%d, removing subscription", child->m_topic.c_str(), child->m_partition);
                child->m_trigger.set(true);
                it = m_subscriptions.erase(it);
                continue;
            }
            ++it;
            continue;
        }

        LOG_CORE("received result %d for partition consumer %s:%d", result, child->m_topic.c_str(), child->m_partition);
        child->m_preferred_read_replica = invalidPreferredReplicaID;

        if (result == ErrTimedOut)
        {
            LOG_CORE("received timeout for %s:%d, removing subscription", child->m_topic.c_str(), child->m_partition);
            it = m_subscriptions.erase(it);
        }
        else if (result == ErrOffsetOutOfRange)
        {
            LOG_CORE("received OffsetOutOfRange for %s:%d, sending error and removing subscription", child->m_topic.c_str(), child->m_partition);
            child->SendError(result);
            child->m_trigger.set(true);
            it = m_subscriptions.erase(it);
        }
        else if (result == ErrUnknownTopicOrPartition || result == ErrNotLeaderForPartition ||
                 result == ErrLeaderNotAvailable || result == ErrReplicaNotAvailable ||
                 result == ErrFencedLeaderEpoch || result == ErrUnknownLeaderEpoch)
        {
            LOG_CORE("received broker error %d for %s:%d, removing subscription", result, child->m_topic.c_str(), child->m_partition);
            child->m_trigger.set(true);
            it = m_subscriptions.erase(it);
        }
        else
        {
            LOG_CORE("received other error %d for %s:%d, sending error and removing subscription", result, child->m_topic.c_str(), child->m_partition);
            child->SendError(result);
            child->m_trigger.set(true);
            it = m_subscriptions.erase(it);
        }
    }
}

coev::awaitable<void> BrokerConsumer::Abort(int err)
{
    LOG_CORE("BrokerConsumer::Abort called with error %d, abandoning broker consumer", err);
    m_consumer->AbandonBrokerConsumer(shared_from_this());
    co_await m_broker->Close();

    LOG_CORE("BrokerConsumer::Abort notifying %zu active subscriptions of error", m_subscriptions.size());
    for (auto &child : m_subscriptions)
    {
        child.first->SendError(err);
        child.first->m_trigger.set(true);
    }

    if (m_new_subscriptions.empty())
    {
        LOG_CORE("BrokerConsumer::Abort no new subscriptions, sleeping");
        co_await sleep_for(PartitionConsumersBatchTimeout);
    }
    else
    {
        LOG_CORE("BrokerConsumer::Abort notifying %zu new subscriptions of error", m_new_subscriptions.size());
        for (auto &child : m_new_subscriptions)
        {
            child->SendError(err);
            child->m_trigger.set(true);
        }
    }
    LOG_CORE("BrokerConsumer::Abort finished");
}

coev::awaitable<int> BrokerConsumer::FetchNewMessages(std::shared_ptr<FetchResponse> &response)
{
    LOG_CORE("BrokerConsumer::FetchNewMessages starting");
    FetchRequest request;
    request.m_min_bytes = m_consumer->m_conf->Consumer.Fetch.Min;
    request.m_max_wait_time = static_cast<int32_t>(m_consumer->m_conf->Consumer.MaxWaitTime.count() / std::chrono::milliseconds(1).count());

    if (m_consumer->m_conf->Version.IsAtLeast(V0_9_0_0))
    {
        request.m_version = 1;
    }
    if (m_consumer->m_conf->Version.IsAtLeast(V0_10_0_0))
    {
        request.m_version = 2;
    }
    if (m_consumer->m_conf->Version.IsAtLeast(V0_10_1_0))
    {
        request.m_version = 3;
        request.m_max_bytes = MaxResponseSize;
    }
    if (m_consumer->m_conf->Version.IsAtLeast(V0_11_0_0))
    {
        request.m_version = 5;
        request.m_isolation = m_consumer->m_conf->Consumer.IsolationLevel_;
    }
    if (m_consumer->m_conf->Version.IsAtLeast(V1_0_0_0))
    {
        request.m_version = 6;
    }
    if (m_consumer->m_conf->Version.IsAtLeast(V1_1_0_0))
    {
        request.m_version = 7;
        request.m_session_id = 0;
        request.m_session_epoch = -1;
    }
    if (m_consumer->m_conf->Version.IsAtLeast(V2_0_0_0))
    {
        request.m_version = 8;
    }
    if (m_consumer->m_conf->Version.IsAtLeast(V2_1_0_0))
    {
        request.m_version = 10;
    }
    if (m_consumer->m_conf->Version.IsAtLeast(V2_3_0_0))
    {
        request.m_version = 11;
        request.m_rack_id = m_consumer->m_conf->RackId;
    }

    for (auto &child : m_subscriptions)
    {
        if (!child.first->IsPaused())
        {
            request.AddBlock(child.first->m_topic, child.first->m_partition, child.first->m_offset, child.first->m_fetch_size, child.first->m_leader_epoch);
        }
    }

    if (request.m_blocks.empty())
    {
        LOG_CORE("BrokerConsumer::FetchNewMessages no blocks to fetch, returning");
        co_return 0;
    }

    LOG_CORE("BrokerConsumer::FetchNewMessages sending FetchRequest with %zu blocks", request.m_blocks.size());
    ResponsePromise<FetchResponse> local_response;
    int err = co_await m_broker->Fetch(std::make_shared<FetchRequest>(request), local_response);
    if (err == 0)
    {
        LOG_CORE("BrokerConsumer::FetchNewMessages received successful response, creating shared_ptr");
        response = std::make_shared<FetchResponse>(std::move(local_response.m_response));
    }
    else
    {
        LOG_CORE("BrokerConsumer::FetchNewMessages Fetch failed with error %d", err);
    }
    co_return err;
}

std::shared_ptr<BrokerConsumer> NewBrokerConsumer(std::shared_ptr<Consumer> c, std::shared_ptr<Broker> m_broker)
{
    auto bc = std::make_shared<BrokerConsumer>();
    bc->m_consumer = c;
    bc->m_broker = m_broker;
    bc->m_refs = 0;

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