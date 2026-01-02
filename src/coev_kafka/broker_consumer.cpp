#include "broker_consumer.h"
#include "partition_consumer.h"
#include "response_header.h"
#include "sleep_for.h"

auto partitionConsumersBatchTimeout = std::chrono::milliseconds(100);

coev::awaitable<void> BrokerConsumer::subscriptionManager()
{
    while (true)
    {
        std::vector<std::shared_ptr<PartitionConsumer>> partitionConsumers;

        auto pc = co_await input.get();
        if (!pc)
            co_return;
        partitionConsumers.emplace_back(pc);

        auto timerEnd = std::chrono::steady_clock::now() + partitionConsumersBatchTimeout;
        while (std::chrono::steady_clock::now() < timerEnd)
        {

            if (pc = co_await input.get())
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

        newSubscriptions.move(std::move(partitionConsumers));
    }
}

coev::awaitable<void> BrokerConsumer::subscriptionConsumer()
{

    while (true)
    {
        std::vector<std::shared_ptr<PartitionConsumer>> sub = co_await newSubscriptions.get();
        updateSubscriptions(sub);

        if (subscriptions.empty())
        {
            co_await sleep_for(partitionConsumersBatchTimeout);
            continue;
        }

        std::shared_ptr<FetchResponse> response;
        int err = co_await fetchNewMessages(response);
        if (err != 0)
        {
            abort(err);
            co_return;
        }

        if (!response)
        {
            co_await sleep_for(partitionConsumersBatchTimeout);
            co_return;
        }

        for (auto &child : subscriptions)
        {
            auto it1 = response->Blocks.find(child.first->topic);
            if (it1 == response->Blocks.end())
            {
                continue;
            }
            auto it2 = it1->second.find(child.first->partition);
            if (it2 == it1->second.end())
            {
                continue;
            }
            child.first->feeder.set(response);
        }

        co_await handleResponses();
    }
}
void BrokerConsumer::updateSubscriptions(const std::vector<std::shared_ptr<PartitionConsumer>> &newSubscriptions)
{
    for (auto &child : newSubscriptions)
    {
        subscriptions[child] = true;
    }

    for (auto it = subscriptions.begin(); it != subscriptions.end();)
    {
        bool found = false;
        auto child = it->first;
        if (child->dying.try_get(found))
        {
            child->trigger.set(true);
            it = subscriptions.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

coev::awaitable<void> BrokerConsumer::handleResponses()
{
    for (auto it = subscriptions.begin(); it != subscriptions.end();)
    {
        auto child = it->first;
        auto result = child->responseResult;
        child->responseResult = ErrNoError;

        if (!result)
        {
            std::shared_ptr<Broker> preferredBroker;
            int _;
            auto err_code = co_await child->preferredBroker(preferredBroker, _);
            if (err_code == 0 && broker->ID() != preferredBroker->ID())
            {
                child->trigger.set(true);
                it = subscriptions.erase(it);
                continue;
            }
            ++it;
            continue;
        }

        child->preferredReadReplica = invalidPreferredReplicaID;

        if (result == ErrTimedOut)
        {
            it = subscriptions.erase(it);
        }
        else if (result == ErrOffsetOutOfRange)
        {
            child->SendError(result);
            child->trigger.set(true);
            it = subscriptions.erase(it);
        }
        else if (result == ErrUnknownTopicOrPartition || result == ErrNotLeaderForPartition ||
                 result == ErrLeaderNotAvailable || result == ErrReplicaNotAvailable ||
                 result == ErrFencedLeaderEpoch || result == ErrUnknownLeaderEpoch)
        {
            child->trigger.set(true);
            it = subscriptions.erase(it);
        }
        else
        {
            child->SendError(result);
            child->trigger.set(true);
            it = subscriptions.erase(it);
        }
    }
}

coev::awaitable<void> BrokerConsumer::abort(int err)
{
    consumer_->abandonBrokerConsumer(shared_from_this());
    broker->Close();

    for (auto &child : subscriptions)
    {
        child.first->SendError(err);
        child.first->trigger.set(true);
    }

    while (true)
    {
        auto _subscriptions = co_await newSubscriptions.get();
        if (_subscriptions.empty())
        {
            co_await sleep_for(partitionConsumersBatchTimeout);
            continue;
        }
        for (auto &child : _subscriptions)
        {
            child->SendError(err);
            child->trigger.set(true);
        }
    }
}

coev::awaitable<int> BrokerConsumer::fetchNewMessages(std::shared_ptr<FetchResponse> &response)
{
    auto request = std::make_shared<FetchRequest>();
    request->MinBytes = consumer_->conf->Consumer.Fetch.Min;
    request->MaxWaitTime = static_cast<int32_t>(consumer_->conf->Consumer.MaxWaitTime.count() / std::chrono::milliseconds(1).count());

    if (consumer_->conf->Version.IsAtLeast(V0_9_0_0))
    {
        request->Version = 1;
    }
    if (consumer_->conf->Version.IsAtLeast(V0_10_0_0))
    {
        request->Version = 2;
    }
    if (consumer_->conf->Version.IsAtLeast(V0_10_1_0))
    {
        request->Version = 3;
        request->MaxBytes = MaxResponseSize;
    }
    if (consumer_->conf->Version.IsAtLeast(V0_11_0_0))
    {
        request->Version = 5;
        request->Isolation = consumer_->conf->Consumer.IsolationLevel_;
    }
    if (consumer_->conf->Version.IsAtLeast(V1_0_0_0))
    {
        request->Version = 6;
    }
    if (consumer_->conf->Version.IsAtLeast(V1_1_0_0))
    {
        request->Version = 7;
        request->SessionID = 0;
        request->SessionEpoch = -1;
    }
    if (consumer_->conf->Version.IsAtLeast(V2_0_0_0))
    {
        request->Version = 8;
    }
    if (consumer_->conf->Version.IsAtLeast(V2_1_0_0))
    {
        request->Version = 10;
    }
    if (consumer_->conf->Version.IsAtLeast(V2_3_0_0))
    {
        request->Version = 11;
        request->RackID = consumer_->conf->RackId;
    }

    for (auto &child : subscriptions)
    {
        if (!child.first->IsPaused())
        {
            request->AddBlock(child.first->topic, child.first->partition, child.first->offset, child.first->fetchSize, child.first->leaderEpoch);
        }
    }

    if (request->blocks.empty())
    {
        response = nullptr;
        co_return 0;
    }

    co_return co_await broker->Fetch(request, response);
}

std::shared_ptr<BrokerConsumer> NewBrokerConsumer(std::shared_ptr<Consumer> c, std::shared_ptr<Broker> broker)
{
    auto bc = std::make_shared<BrokerConsumer>();
    bc->consumer_ = c;
    bc->broker = broker;
    bc->refs = 0;

    bc->task_ << bc->subscriptionManager();
    bc->task_ << bc->subscriptionConsumer();

    return bc;
}

int BrokerConsumer::Topics(std::vector<std::string> &out)
{
    return consumer_->Topics(out);
}
coev::awaitable<int> BrokerConsumer::Partitions(const std::string &topic, std::vector<int32_t> &out)
{
    return consumer_->Partitions(topic, out);
}
coev::awaitable<int> BrokerConsumer::Close()
{
    return consumer_->Close();
}
std::map<std::string, std::map<int32_t, int64_t>> BrokerConsumer::HighWaterMarks()
{
    return consumer_->HighWaterMarks();
}

void BrokerConsumer::Pause(const std::map<std::string, std::vector<int32_t>> &topicPartitions)
{
    consumer_->Pause(topicPartitions);
}
void BrokerConsumer::Resume(const std::map<std::string, std::vector<int32_t>> &topicPartitions)
{
    consumer_->Resume(topicPartitions);
}
void BrokerConsumer::PauseAll()
{
    consumer_->PauseAll();
}
void BrokerConsumer::ResumeAll()
{
    consumer_->ResumeAll();
}