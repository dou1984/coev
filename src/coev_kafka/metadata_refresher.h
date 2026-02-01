#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <coev/coev.h>
#include <coev/coev.h>
#include "errors.h"

struct Client;
struct MetadataRefresher
{
    struct RefreshTime
    {
        std::chrono::time_point<std::chrono::system_clock> m_last_request_time;
        std::chrono::time_point<std::chrono::system_clock> m_last_response_time;
    };
    MetadataRefresher();

    std::vector<std::string> add_topics(const std::vector<std::string> &topics);
    void update_topic(const std::string &topic);
    bool has_topics(const std::vector<std::string> &topics);

    void clear();

    std::unordered_map<std::string, RefreshTime> m_topics_map;
    std::vector<std::string> m_topics;
    std::chrono::milliseconds m_refresh_interval_mininum{100};
};