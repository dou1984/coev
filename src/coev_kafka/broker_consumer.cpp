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

BrokerConsumer::BrokerConsumer(std::shared_ptr<Consumer> c, std::shared_ptr<Broker> broker)
    : m_consumer(c), m_broker(broker)
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
    auto next_wakeup = std::chrono::system_clock::now();
    while (true)
    {
        UpdateSubscriptions();
        if (m_subscriptions.empty())
        {
            next_wakeup += PartitionConsumersBatchTimeout;
            auto now = std::chrono::system_clock::now();
            if (next_wakeup > now)
            {
                auto sleep_ms = std::chrono::duration_cast<std::chrono::milliseconds>(next_wakeup - now);
                if (sleep_ms.count() < 10)
                {
                    co_await sleep_for(std::chrono::milliseconds(10));
                }
                else
                {
                    co_await sleep_for(sleep_ms);
                }
            }
        }
        auto response = std::make_shared<FetchResponse>();
        int err = co_await FetchNewMessages(response);
        if (err != 0)
        {
            LOG_CORE("BrokerConsumer::SubscriptionConsumer FetchNewMessages failed with error %d, aborting", err);
            co_await Abort(err);
            co_return;
        }

        for (auto &[child, _] : m_subscriptions)
        {
            auto tit = response->m_blocks.find(child->m_topic);
            if (tit == response->m_blocks.end())
            {
                continue;
            }
            auto pit = tit->second.find(child->m_partition);
            if (pit == tit->second.end())
            {
                continue;
            }
            child->m_feeder.set(response);
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

    m_new_subscriptions.clear();

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

        if (result == ErrNoError)
        {
            LOG_CORE("no result for partition consumer %s:%d, checking preferred broker", child->m_topic.c_str(), child->m_partition);
            std::shared_ptr<Broker> _broker;
            int _;
            auto err_code = co_await child->PreferredBroker(_broker, _);
            if (err_code == 0 && m_broker->ID() != _broker->ID())
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
        child->m_preferred_read_replica = InvalidPreferredReplicaID;

        if (result == ErrTimedOut)
        {
            LOG_CORE("received timeout for %s:%d, removing subscription", child->m_topic.c_str(), child->m_partition);
            it = m_subscriptions.erase(it);
        }
        else if (result == ErrOffsetOutOfRange)
        {
            auto err = co_await child->ChooseStartingOffset(child->m_conf->Consumer.Offsets.Initial);
            if (err != ErrNoError)
            {
                LOG_CORE("failed to reset offset for %s:%d, error %d", child->m_topic.c_str(), child->m_partition, err);
                child->SendError(err);
                child->m_trigger.set(true);
                it = m_subscriptions.erase(it);
            }
            else
            {
                LOG_CORE("successfully reset offset for %s:%d", child->m_topic.c_str(), child->m_partition);
                child->m_trigger.set(true);
                ++it;
            }
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
    m_consumer->AbandonBrokerConsumer(shared_from_this());

    LOG_CORE("BrokerConsumer::Abort notifying %zu active subscriptions of error", m_subscriptions.size());
    for (auto &[child, _] : m_subscriptions)
    {
        child->SendError(err);
        child->m_trigger.set(true);
    }
    if (m_new_subscriptions.empty())
    {
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

    m_broker->Close();
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

    for (auto &[child, _] : m_subscriptions)
    {
        if (!child->IsPaused())
        {
            request->add_block(child->m_topic, child->m_partition, child->m_offset, child->m_fetch_size, child->m_leader_epoch);
        }
    }

    if (request->m_blocks.empty())
    {
        LOG_CORE("BrokerConsumer::FetchNewMessages no blocks to fetch, returning");
        co_return 0;
    }

    ResponsePromise<FetchResponse> local_response;
    int err = co_await m_broker->Fetch(request, local_response);
    if (err == 0)
    {
        LOG_CORE("BrokerConsumer::FetchNewMessages received successful response, creating shared_ptr");
        response = local_response.m_response;
    }
    else
    {
        LOG_CORE("BrokerConsumer::FetchNewMessages received error response %d", err);
    }
    co_return err;
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