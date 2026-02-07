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
    ConfigResource() = default;
    ConfigResource(ConfigResourceType type, const std::string &name) : m_type(type), m_name(name)
    {
    }
};

std::string toString(ConfigResourceType crt);