#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <coev/coev.h>
#include <coev/coev.h>
#include "errors.h"

using MetadataRefresh = std::function<coev::awaitable<int>(const std::vector<std::string> &)>;

struct NextRefresh;

struct CurrentRefresh
{

    CurrentRefresh(MetadataRefresh refresh_func);

    void addTopicsFrom(std::shared_ptr<NextRefresh> next);
    void addTopics(const std::vector<std::string> &topics);
    bool hasTopics(const std::vector<std::string> &topics);
    void start();
    void wait();
    void clear();

    std::mutex mu;
    bool ongoing;
    std::unordered_map<std::string, bool> topicsMap;
    std::vector<std::string> topics;
    bool allTopics;
    MetadataRefresh refresh;
    coev::co_channel<int> chans;
};