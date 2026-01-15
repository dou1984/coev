#include "version.h"
#include <mutex>
#include <regex>
#include <sstream>
#include <cstring>

std::string version()
{
    static std::string v = "dev";
    // static std::string v = V0_10_0_0.String();
    return v;
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
const std::vector<KafkaVersion> SupportedVersions = {
    V0_8_2_0, V0_8_2_1, V0_8_2_2, V0_9_0_0, V0_9_0_1,
    V0_10_0_0, V0_10_0_1, V0_10_1_0, V0_10_1_1, V0_10_2_0,
    V0_10_2_1, V0_10_2_2, V0_11_0_0, V0_11_0_1, V0_11_0_2,
    V1_0_0_0, V1_0_1_0, V1_0_2_0, V1_1_0_0, V1_1_1_0,
    V2_0_0_0, V2_0_1_0, V2_1_0_0, V2_1_1_0, V2_2_0_0,
    V2_2_1_0, V2_2_2_0, V2_3_0_0, V2_3_1_0, V2_4_0_0,
    V2_4_1_0, V2_5_0_0, V2_5_1_0, V2_6_0_0, V2_6_1_0,
    V2_6_2_0, V2_6_3_0, V2_7_0_0, V2_7_1_0, V2_7_2_0,
    V2_8_0_0, V2_8_1_0, V2_8_2_0, V3_0_0_0, V3_0_1_0,
    V3_0_2_0, V3_1_0_0, V3_1_1_0, V3_1_2_0, V3_2_0_0,
    V3_2_1_0, V3_2_2_0, V3_2_3_0, V3_3_0_0, V3_3_1_0,
    V3_3_2_0, V3_4_0_0, V3_4_1_0, V3_5_0_0, V3_5_1_0,
    V3_5_2_0, V3_6_0_0, V3_6_1_0, V3_6_2_0, V3_7_0_0,
    V3_7_1_0, V3_7_2_0, V3_8_0_0, V3_8_1_0, V3_9_0_0,
    V3_9_1_0, V4_0_0_0, V4_1_0_0};

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