
#include "single_flight_metadata_refresher.h"

MetadataRefresh singleFlightMetadataRefresher::NewSingleFlightRefresher(MetadataRefresh f)
{
    auto refresher = NewMetadataRefresh(f);
    return [refresher](const std::vector<std::string> &topics) -> coev::awaitable<int>
    {
        return refresher->Refresh(topics);
    };
}

std::shared_ptr<singleFlightMetadataRefresher> singleFlightMetadataRefresher::NewMetadataRefresh(MetadataRefresh f)
{
    auto current_ = std::make_shared<CurrentRefresh>(f);
    auto next_ = std::make_shared<NextRefresh>();
    return std::make_shared<singleFlightMetadataRefresher>(singleFlightMetadataRefresher{current_, next_});
}

coev::awaitable<int> singleFlightMetadataRefresher::Refresh(const std::vector<std::string> &topics)
{
    while (true)
    {
        coev::co_channel<int> ch;
        auto queued = RefreshOrQueue(topics, ch);
        if (!queued)
        {
            co_return co_await ch.get();
        }
        co_await ch.get();
    }
}

bool singleFlightMetadataRefresher::RefreshOrQueue(const std::vector<std::string> &topics, coev::co_channel<int> &ch)
{
    std::lock_guard<std::mutex> lock_current(Current->mu);
    if (!Current->ongoing)
    {
        std::lock_guard<std::mutex> lock_next(Next->mu);
        Current->addTopicsFrom(Next);
        Next->clear();
        Current->addTopics(topics);

        return false;
    }

    if (Current->hasTopics(topics))
    {

        return false;
    }

    {
        std::lock_guard<std::mutex> lock_next(Next->mu);
        Next->addTopics(topics);
    }
    return true;
}