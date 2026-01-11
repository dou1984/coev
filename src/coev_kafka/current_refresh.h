#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <coev/coev.h>
#include <coev/coev.h>
#include "errors.h"

using fMetadataRefresh = std::function<coev::awaitable<int>(const std::vector<std::string> &)>;

struct NextRefresh;

struct CurrentRefresh
{

    CurrentRefresh(fMetadataRefresh refresh_func);

    void addTopicsFrom(std::shared_ptr<NextRefresh> next);
    void addTopics(const std::vector<std::string> &topics);
    bool hasTopics(const std::vector<std::string> &topics);
    void start();
    void wait();
    void clear();

    std::mutex m_mutex;
    bool m_ongoing = false;
    std::unordered_map<std::string, bool> m_topics_map;
    std::vector<std::string> m_topics;
    bool m_all_topics = false;
    fMetadataRefresh m_refresh_func;
    coev::co_channel<int> m_chans;
};