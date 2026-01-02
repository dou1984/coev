
#pragma once

#include <string>
#include <vector>
#include <mutex>

struct NextRefresh
{

    void addTopics(const std::vector<std::string> &topics);
    void clear();

    std::mutex mu;
    std::vector<std::string> topics;
    bool allTopics = false;
};