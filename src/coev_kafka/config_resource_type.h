#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum ConfigResourceType : int8_t
{
    UnknownResource = 0,
    TopicResource = 2,
    BrokerResource = 4,
    BrokerLoggerResource = 8,
};

struct ConfigResource
{
    ConfigResourceType Type;
    std::string Name;
    std::vector<std::string> ConfigNames;
};

std::string toString(ConfigResourceType crt);