#include "version.h"
#include "delete_offsets_response.h"

void DeleteOffsetsResponse::set_version(int16_t v)
{
    m_version = v;
}

void DeleteOffsetsResponse::AddError(const std::string &topic, int32_t partition, KError errorCode)
{
    auto &partitions = m_errors[topic];
    partitions[partition] = errorCode;
}

int DeleteOffsetsResponse::encode(packetEncoder &pe)
{
    pe.putInt16(static_cast<int16_t>(m_error_code));
    pe.putDurationMs(m_throttle_time);

    if (pe.putArrayLength(static_cast<int32_t>(m_errors.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &topicEntry : m_errors)
    {
        const std::string &topic = topicEntry.first;
        auto &partitions = topicEntry.second;

        if (pe.putString(topic) != ErrNoError)
        {
            return ErrEncodeError;
        }
        if (pe.putArrayLength(static_cast<int32_t>(partitions.size())) != ErrNoError)
        {
            return ErrEncodeError;
        }

        for (auto &partEntry : partitions)
        {
            int32_t partition = partEntry.first;
            KError err = partEntry.second;

            pe.putInt32(partition);
            pe.putInt16(static_cast<int16_t>(err));
        }
    }

    return ErrNoError;
}

int DeleteOffsetsResponse::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;

    int16_t errCode;
    if (pd.getInt16(errCode) != ErrNoError)
    {
        return ErrDecodeError;
    }
    m_error_code = static_cast<KError>(errCode);

    if (pd.getDurationMs(m_throttle_time) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t numTopics;
    if (pd.getArrayLength(numTopics) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (numTopics <= 0)
    {
        m_errors.clear();
        return ErrNoError;
    }

    m_errors.clear();
    m_errors.reserve(numTopics);

    for (int32_t i = 0; i < numTopics; ++i)
    {
        std::string topic;
        if (pd.getString(topic) != ErrNoError)
        {
            return ErrDecodeError;
        }

        int32_t numPartitions;
        if (pd.getArrayLength(numPartitions) != ErrNoError)
        {
            return ErrDecodeError;
        }

        auto &partitionMap = m_errors[topic];
        partitionMap.clear();

        for (int32_t j = 0; j < numPartitions; ++j)
        {
            int32_t partition;
            if (pd.getInt32(partition) != ErrNoError)
            {
                return ErrDecodeError;
            }

            int16_t partErr;
            if (pd.getInt16(partErr) != ErrNoError)
            {
                return ErrDecodeError;
            }
            partitionMap[partition] = static_cast<KError>(partErr);
        }
    }

    return ErrNoError;
}

int16_t DeleteOffsetsResponse::key() const
{
    return 47; // apiKeyOffsetDelete
}

int16_t DeleteOffsetsResponse::version() const
{
    return m_version;
}

int16_t DeleteOffsetsResponse::header_version() const
{
    return 0;
}

bool DeleteOffsetsResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 2;
}

KafkaVersion DeleteOffsetsResponse::required_version() const
{
    return V2_4_0_0;
}

std::chrono::milliseconds DeleteOffsetsResponse::throttle_time() const
{
    return m_throttle_time;
}