
#pragma once

#include <string>
#include <vector>
#include <mutex>

struct NextRefresh
{

    void addTopics(const std::vector<std::string> &topics);
    void clear();

    std::mutex m_mutex;
    std::vector<std::string> m_topics;
    bool m_all_topics = false;
};