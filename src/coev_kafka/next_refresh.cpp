
#include "next_refresh.h"

void NextRefresh::addTopics(const std::vector<std::string> &topics_)
{
    if (topics_.empty())
    {
        m_all_topics = true;
        m_topics.clear();
        return;
    }
    m_topics.insert(m_topics.end(), topics_.begin(), topics_.end());
}

void NextRefresh::clear()
{
    m_topics.clear();
    m_all_topics = false;
}