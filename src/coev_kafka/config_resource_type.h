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
    ConfigResourceType m_type;
    std::string m_name;
    std::vector<std::string> m_config_names;
};

std::string toString(ConfigResourceType crt);