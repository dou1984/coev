/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include "version.h"
#include <mutex>
#include <regex>
#include <sstream>
#include <cstring>
#include "config.h"

namespace coev::kafka
{
    std::string version()
    {
#ifdef COEV_VERSION
        static std::string v = COEV_VERSION;
#else
        static std::string v = "dev";
#endif
        return defaultClientSoftwareVersion;
    }

    bool KafkaVersion::IsAtLeast(const KafkaVersion &other) const
    {
        for (int i = 0; i < 4; ++i)
        {
            if (version_[i] > other.version_[i])
                return true;
            if (version_[i] < other.version_[i])
                return false;
        }
        return true;
    }

    std::string KafkaVersion::String() const
    {
        std::ostringstream oss;
        if (version_[0] == 0)
        {
            oss << "0." << version_[1] << "." << version_[2] << "." << version_[3];
        }
        else
        {
            oss << version_[0] << "." << version_[1] << "." << version_[2];
        }
        return oss.str();
    }

    bool KafkaVersion::operator==(const KafkaVersion &other) const
    {
        return std::equal(version_, version_ + 4, other.version_);
    }

    bool KafkaVersion::operator!=(const KafkaVersion &other) const
    {
        return !(*this == other);
    }
    bool KafkaVersion::operator>=(const KafkaVersion &o) const
    {
        return version_[0] >= o.version_[0] || (version_[0] == o.version_[0] && version_[1] >= o.version_[1]) || (version_[1] == o.version_[1] && version_[2] >= o.version_[2]) || (version_[2] == o.version_[2] && version_[3] >= o.version_[3]);
    }

    KafkaVersion ParseKafkaVersion(const std::string &s)
    {
        static const std::regex validPreKafka1Version(R"(^0\.\d+\.\d+\.\d+$)");
        static const std::regex validPostKafka1Version(R"(^\d+\.\d+\.\d+$)");

        if (s.length() < 5)
        {
            return DefaultVersion;
        }

        uint major = 0, minor = 0, veryMinor = 0, patch = 0;

        if (s[0] == '0')
        {
            if (!std::regex_match(s, validPreKafka1Version))
            {
                return DefaultVersion;
            }
            sscanf(s.c_str(), "0.%u.%u.%u", &minor, &veryMinor, &patch);
        }
        else
        {
            if (!std::regex_match(s, validPostKafka1Version))
            {
                return DefaultVersion;
            }
            sscanf(s.c_str(), "%u.%u.%u", &major, &minor, &veryMinor);
        }

        return KafkaVersion(major, minor, veryMinor, patch);
    }
}