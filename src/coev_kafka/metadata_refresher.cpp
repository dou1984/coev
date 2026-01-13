#include "metadata_refresher.h"

metadata_refresher::metadata_refresher()
{
}

void metadata_refresher::set_refresher(fRefresher f)
{
    m_refresh_func = f;
}
std::vector<std::string> metadata_refresher::add_topics(const std::vector<std::string> &topics)
{
    std::vector<std::string> refresh_topics;
    auto now = std::chrono::system_clock::now();
    for (const auto &topic : topics)
    {
        auto it = m_topics_map.find(topic);
        if (it != m_topics_map.end())
        {
            if (it->second.m_last_response_time + m_refresh_interval_mininum > now)
            {
                continue;
            }
            if (it->second.m_last_request_time + m_refresh_interval_mininum > now)
            {
                continue;
            }
            refresh_topics.push_back(topic);
            it->second.m_last_request_time = now;
        }
        else
        {
            m_topics_map[topic] = {now, now};
            m_topics.push_back(topic);
            refresh_topics.push_back(topic);
        }
    }
    return refresh_topics;
}

bool metadata_refresher::has_topics(const std::vector<std::string> &topics)
{
    for (const auto &topic : topics)
    {
        if (m_topics_map.find(topic) == m_topics_map.end())
        {
            return false;
        }
    }
    return true;
}

void metadata_refresher::clear()
{
    m_topics.clear();
    m_topics_map.clear();
}

coev::awaitable<int> metadata_refresher::refresh(const std::vector<std::string> &topics)
{
    assert(m_refresh_func != nullptr);
    return m_refresh_func(topics);
}
