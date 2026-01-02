
#include "next_refresh.h"

void NextRefresh::addTopics(const std::vector<std::string> &topics_)
{
    if (topics_.empty())
    {
        allTopics = true;
        topics.clear();
        return;
    }
    topics.insert(topics.end(), topics_.begin(), topics_.end());
}

void NextRefresh::clear()
{
    topics.clear();
    allTopics = false;
}