#include "current_refresh.h"
#include "next_refresh.h"

CurrentRefresh::CurrentRefresh(MetadataRefresh refresh_func)
    : m_ongoing(false), m_all_topics(false), m_refresh_func(std::move(refresh_func)) {}

void CurrentRefresh::addTopicsFrom(std::shared_ptr<NextRefresh> next)
{
    if (next->m_all_topics)
    {
        m_all_topics = true;
        return;
    }
    if (!next->m_topics.empty())
    {
        addTopics(next->m_topics);
    }
}

void CurrentRefresh::addTopics(const std::vector<std::string> &topics)
{
    if (topics.empty())
    {
        m_all_topics = true;
        return;
    }
    for (const auto &topic : topics)
    {
        if (m_topics_map.find(topic) != m_topics_map.end())
        {
            continue;
        }
        m_topics_map[topic] = true;
        m_topics.push_back(topic);
    }
}

bool CurrentRefresh::hasTopics(const std::vector<std::string> &topics)
{
    if (topics.empty())
    {
        return m_all_topics;
    }
    if (m_all_topics)
    {
        return true;
    }
    for (const auto &topic : topics)
    {
        if (m_topics_map.find(topic) == m_topics_map.end())
        {
            return false;
        }
    }
    return true;
}

void CurrentRefresh::start()
{

    std::vector<std::string> topics_to_refresh = m_topics;
    if (m_all_topics)
    {
        topics_to_refresh.clear();
    }

    co_start << [this, topics_to_refresh]() -> coev::awaitable<void>
    {
        auto err = co_await this->m_refresh_func(topics_to_refresh);
        std::lock_guard<std::mutex> lock(m_mutex);
        m_ongoing = false;
        m_chans.set(err);
        clear();
    }();
}

void CurrentRefresh::clear()
{
    m_topics.clear();
    m_topics_map.clear();
    m_all_topics = false;
}

void CurrentRefresh::wait()
{
}