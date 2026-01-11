// single_flight_metadata_refresher.h
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <coev/coev.h>
#include "current_refresh.h"
#include "next_refresh.h"

struct SingleFlightMetadataRefresher
{
    SingleFlightMetadataRefresher(std::shared_ptr<CurrentRefresh> current, std::shared_ptr<NextRefresh> next) : m_current(current), m_next(next)
    {
    }
    coev::awaitable<int> Refresh(const std::vector<std::string> &topics);
    bool RefreshOrQueue(const std::vector<std::string> &topics, coev::co_channel<int> &ch);

    std::shared_ptr<CurrentRefresh> m_current;
    std::shared_ptr<NextRefresh> m_next;
};

fMetadataRefresh NewSingleFlightRefresher(fMetadataRefresh f);