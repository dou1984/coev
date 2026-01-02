// single_flight_metadata_refresher.h
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <coev.h>
#include "current_refresh.h"
#include "next_refresh.h"

struct singleFlightMetadataRefresher
{

    static MetadataRefresh NewSingleFlightRefresher(MetadataRefresh f);
    static std::shared_ptr<singleFlightMetadataRefresher> NewMetadataRefresh(MetadataRefresh f);

    coev::awaitable<int> Refresh(const std::vector<std::string> &topics);
    bool RefreshOrQueue(const std::vector<std::string> &topics, coev::co_channel<int> &ch);

    std::shared_ptr<CurrentRefresh> Current;
    std::shared_ptr<NextRefresh> Next;
};