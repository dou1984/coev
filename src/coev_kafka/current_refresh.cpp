#include "current_refresh.h"
#include "next_refresh.h"

CurrentRefresh::CurrentRefresh(MetadataRefresh refresh_func)
    : ongoing(false), allTopics(false), refresh(std::move(refresh_func)) {}

void CurrentRefresh::addTopicsFrom(std::shared_ptr<NextRefresh> next)
{
    if (next->allTopics)
    {
        allTopics = true;
        return;
    }
    if (!next->topics.empty())
    {
        addTopics(next->topics);
    }
}

void CurrentRefresh::addTopics(const std::vector<std::string> &topics)
{
    if (topics.empty())
    {
        allTopics = true;
        return;
    }
    for (const auto &topic : topics)
    {
        if (topicsMap.find(topic) != topicsMap.end())
        {
            continue;
        }
        topicsMap[topic] = true;
        this->topics.push_back(topic);
    }
}

bool CurrentRefresh::hasTopics(const std::vector<std::string> &topics)
{
    if (topics.empty())
    {
        return allTopics;
    }
    if (allTopics)
    {
        return true;
    }
    for (const auto &topic : topics)
    {
        if (topicsMap.find(topic) == topicsMap.end())
        {
            return false;
        }
    }
    return true;
}

void CurrentRefresh::start()
{

    std::vector<std::string> topics_to_refresh = topics;
    if (allTopics)
    {
        topics_to_refresh.clear();
    }

    co_start << [this, topics_to_refresh]() -> coev::awaitable<void>
    {
        auto err = co_await this->refresh(topics_to_refresh);
        std::lock_guard<std::mutex> lock(mu);
        ongoing = false;
        chans.set(err);
        clear();
    }();
}

void CurrentRefresh::clear()
{
    topics.clear();
    topicsMap.clear();
    allTopics = false;
}

void CurrentRefresh::wait()
{
}