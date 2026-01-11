
#include "single_flight_metadata_refresher.h"

coev::awaitable<int> SingleFlightMetadataRefresher::Refresh(const std::vector<std::string> &topics)
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

bool SingleFlightMetadataRefresher::RefreshOrQueue(const std::vector<std::string> &topics, coev::co_channel<int> &ch)
{
    std::lock_guard<std::mutex> lock_current(m_current->m_mutex);
    if (!m_current->m_ongoing)
    {
        std::lock_guard<std::mutex> lock_next(m_next->m_mutex);
        m_current->addTopicsFrom(m_next);
        m_next->clear();
        m_current->addTopics(topics);

        return false;
    }

    if (m_current->hasTopics(topics))
    {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock_next(m_next->m_mutex);
        m_next->addTopics(topics);
    }
    return true;
}

fMetadataRefresh NewSingleFlightRefresher(fMetadataRefresh f)
{
    auto current_ = std::make_shared<CurrentRefresh>(f);
    auto next_ = std::make_shared<NextRefresh>();
    auto refresher = std::make_shared<SingleFlightMetadataRefresher>(current_, next_);
    return [refresher](const std::vector<std::string> &topics) -> coev::awaitable<int>
    {
        return refresher->Refresh(topics);
    };
}
