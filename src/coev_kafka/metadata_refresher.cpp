#include "metadata_refresher.h"

MetadataRefresher::MetadataRefresher()
{
}

std::vector<std::string> MetadataRefresher::add_topics(const std::vector<std::string> &topics)
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
void MetadataRefresher::update_topic(const std::string &topic)
{
    m_topics_map[topic].m_last_response_time = std::chrono::system_clock::now();
}
bool MetadataRefresher::has_topics(const std::vector<std::string> &topics)
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

void MetadataRefresher::clear()
{
    m_topics.clear();
    m_topics_map.clear();
}
