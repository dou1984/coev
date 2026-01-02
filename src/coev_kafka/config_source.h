#pragma once

#include <cstdint>

enum ConfigSource : int8_t
{
    SourceUnknown = 0,
    SourceTopic = 1,
    SourceDynamicBroker = 2,
    SourceDynamicDefaultBroker = 3,
    SourceStaticBroker = 4,
    SourceDefault = 5
};
