#include "version.h"
#include "offset_fetch_request.h"
#include <stdexcept>
#include "api_versions.h"

OffsetFetchRequest::OffsetFetchRequest()
    : m_version(0), m_require_stable(false)
{
}

OffsetFetchRequest::OffsetFetchRequest(const KafkaVersion &version, const std::string &group, const std::map<std::string, std::vector<int32_t>> &partitions)
    : m_require_stable(false)
{
    m_consumer_group = group;
    m_partitions = partitions;

    if (version.IsAtLeast(V2_5_0_0))
    {
        m_version = 7;
    }
    else if (version.IsAtLeast(V2_4_0_0))
    {
        m_version = 6;
    }
    else if (version.IsAtLeast(V2_1_0_0))
    {
        m_version = 5;
    }
    else if (version.IsAtLeast(V2_0_0_0))
    {
        m_version = 4;
    }
    else if (version.IsAtLeast(V0_11_0_0))
    {
        m_version = 3;
    }
    else if (version.IsAtLeast(V0_10_2_0))
    {
        m_version = 2;
    }
    else if (version.IsAtLeast(V0_8_2_0))
    {
        m_version = 1;
    }
    else
    {
        m_version = 0;
    }
}

void OffsetFetchRequest::set_version(int16_t v)
{
    m_version = v;
}

int OffsetFetchRequest::encode(packetEncoder &pe)
{
    if (m_version < 0 || m_version > 7)
    {
        throw std::runtime_error("invalid or unsupported OffsetFetchRequest version field");
    }

    pe.putString(m_consumer_group);

    if (m_partitions.empty() && m_version >= 2)
    {
        pe.putArrayLength(-1);
    }
    else
    {
        pe.putArrayLength(static_cast<int32_t>(m_partitions.size()));
    }

    for (auto &entry : m_partitions)
    {
        pe.putString(entry.first);
        pe.putInt32Array(entry.second);
        pe.putEmptyTaggedFieldArray();
    }

    if (m_require_stable && m_version < 7)
    {
        throw std::runtime_error("requireStable is not supported. use version 7 or later");
    }

    if (m_version >= 7)
    {
        pe.putBool(m_require_stable);
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int OffsetFetchRequest::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;
    int err = pd.getString(m_consumer_group);
    if (err != 0)
    {
        return err;
    }

    int32_t partitionCount = 0;
    err = pd.getArrayLength(partitionCount);
    if (err != 0)
    {
        return err;
    }
    if ((partitionCount == 0 && version < 2) || partitionCount < 0)
    {
        return ErrInvalidPartitions;
    }

    m_partitions.clear();
    // m_partitions.reserve(partitionCount);
    for (int i = 0; i < partitionCount; ++i)
    {
        std::string topic;
        err = pd.getString(topic);
        if (err != 0)
        {
            return err;
        }
        std::vector<int32_t> parts;
        err = pd.getInt32Array(parts);
        if (err != 0)
        {
            return err;
        }

        int32_t _;
        err = pd.getEmptyTaggedFieldArray(_);
        if (err != 0)
        {
            return err;
        }
        m_partitions[topic] = std::move(parts);
    }

    if (m_version >= 7)
    {
        pd.getBool(m_require_stable);
    }

    int32_t _;
    err = pd.getEmptyTaggedFieldArray(_);
    if (err != 0)
    {
        return err;
    }
    return 0;
}

int16_t OffsetFetchRequest::key() const
{
    return apiKeyOffsetFetch;
}

int16_t OffsetFetchRequest::version() const
{
    return m_version;
}

int16_t OffsetFetchRequest::header_version() const
{
    return m_version >= 6 ? 2 : 1;
}

bool OffsetFetchRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 7;
}

bool OffsetFetchRequest::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool OffsetFetchRequest::is_flexible_version(int16_t version) const
{
    return version >= 6;
}

KafkaVersion OffsetFetchRequest::required_version() const
{
    switch (m_version)
    {
    case 7:
        return V2_5_0_0;
    case 6:
        return V2_4_0_0;
    case 5:
        return V2_1_0_0;
    case 4:
        return V2_0_0_0;
    case 3:
        return V0_11_0_0;
    case 2:
        return V0_10_2_0;
    case 1:
    case 0:
        return V0_8_2_0;
    default:
        return V2_5_0_0;
    }
}

void OffsetFetchRequest::ZeroPartitions()
{
    if (m_partitions.empty() && m_version >= 2)
    {
        m_partitions.clear();
    }
}

void OffsetFetchRequest::AddPartition(const std::string &topic, int32_t partitionID)
{
    m_partitions[topic].push_back(partitionID);
}