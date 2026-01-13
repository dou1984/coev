#include "version.h"
#include "api_versions.h"
#include "describe_log_dirs_response.h"

void DescribeLogDirsResponse::set_version(int16_t v)
{
    m_version = v;
}

int DescribeLogDirsResponse::encode(packetEncoder &pe)
{
    pe.putDurationMs(m_throttle_time);
    if (m_version >= 3)
    {
        pe.putKError(m_error_code);
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_log_dirs.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &dir : m_log_dirs)
    {
        if (dir.encode(pe, m_version) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int DescribeLogDirsResponse::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;
    if (pd.getDurationMs(m_throttle_time) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (version >= 3)
    {
        if (pd.getKError(m_error_code) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_log_dirs.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        if (!m_log_dirs[i].decode(pd, version))
        {
            return ErrDecodeError;
        }
    }

    int32_t dummy;
    if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int16_t DescribeLogDirsResponse::key() const
{
    return apiKeyDescribeLogDirs;
}

int16_t DescribeLogDirsResponse::version() const
{
    return m_version;
}

int16_t DescribeLogDirsResponse::header_version() const
{
    if (m_version >= 2)
    {
        return 1;
    }
    return 0;
}

bool DescribeLogDirsResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 4;
}

bool DescribeLogDirsResponse::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool DescribeLogDirsResponse::is_flexible_version(int16_t version)
{
    return version >= 2;
}

KafkaVersion DescribeLogDirsResponse::required_version() const
{
    switch (m_version)
    {
    case 4:
        return V3_3_0_0;
    case 3:
        return V3_2_0_0;
    case 2:
        return V2_6_0_0;
    case 1:
        return V2_0_0_0;
    default:
        return V1_0_0_0;
    }
}

std::chrono::milliseconds DescribeLogDirsResponse::throttle_time() const
{
    return m_throttle_time;
}

int DescribeLogDirsResponseDirMetadata::encode(packetEncoder &pe, int16_t version)
{
    pe.putKError(m_error_code);

    if (pe.putString(m_path) != ErrNoError)
    {
        return ErrEncodeError;
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_topics.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &topic : m_topics)
    {
        if (topic.encode(pe, version) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    if (version >= 4)
    {
        pe.putInt64(m_total_bytes);
        pe.putInt64(m_usable_bytes);
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int DescribeLogDirsResponseDirMetadata::decode(packetDecoder &pd, int16_t version)
{
    if (pd.getKError(m_error_code) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (pd.getString(m_path) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_topics.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        if (!m_topics[i].decode(pd, version))
        {
            return ErrDecodeError;
        }
    }

    if (version >= 4)
    {
        if (pd.getInt64(m_total_bytes) != ErrNoError)
            return ErrDecodeError;
        if (pd.getInt64(m_usable_bytes) != ErrNoError)
            return ErrDecodeError;
    }

    int32_t dummy;
    if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int DescribeLogDirsResponseTopic::encode(packetEncoder &pe, int16_t version)
{
    if (pe.putString(m_topic) != ErrNoError)
    {
        return ErrEncodeError;
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_partitions.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &partition : m_partitions)
    {
        if (partition.encode(pe, version) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int DescribeLogDirsResponseTopic::decode(packetDecoder &pd, int16_t version)
{
    if (pd.getString(m_topic) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_partitions.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        if (!m_partitions[i].decode(pd, version))
        {
            return ErrDecodeError;
        }
    }

    int32_t dummy;
    if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int DescribeLogDirsResponsePartition::encode(packetEncoder &pe, int16_t version)
{
    pe.putInt32(m_partition_id);
    pe.putInt64(m_size);
    pe.putInt64(m_offset_lag);
    pe.putBool(m_is_temporary);

    if (version >= 2)
    {
        pe.putEmptyTaggedFieldArray();
    }

    return ErrNoError;
}

int DescribeLogDirsResponsePartition::decode(packetDecoder &pd, int16_t version)
{
    if (pd.getInt32(m_partition_id) != ErrNoError)
        return ErrDecodeError;
    if (pd.getInt64(m_size) != ErrNoError)
        return ErrDecodeError;
    if (pd.getInt64(m_offset_lag) != ErrNoError)
        return ErrDecodeError;
    if (pd.getBool(m_is_temporary) != ErrNoError)
        return ErrDecodeError;

    int32_t dummy;
    if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}