#include "version.h"
#include "delete_offsets_request.h"

void DeleteOffsetsRequest::set_version(int16_t v)
{
    m_version = v;
}

int DeleteOffsetsRequest::encode(packetEncoder &pe)
{
    if (pe.putString(m_group) != ErrNoError)
    {
        return ErrEncodeError;
    }

    if (m_partitions.empty())
    {
        pe.putInt32(0); // array length = 0
    }
    else
    {
        if (pe.putArrayLength(static_cast<int32_t>(m_partitions.size())) != ErrNoError)
        {
            return ErrEncodeError;
        }
        for (auto &kv : m_partitions)
        {
            if (pe.putString(kv.first) != ErrNoError)
            {
                return ErrEncodeError;
            }
            if (pe.putInt32Array(kv.second) != ErrNoError)
            {
                return ErrEncodeError;
            }
        }
    }

    return ErrNoError;
}

int DeleteOffsetsRequest::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;

    if (pd.getString(m_group) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t partitionCount;
    if (pd.getArrayLength(partitionCount) != ErrNoError)
    {
        return ErrDecodeError;
    }

    // 根据 Go 逻辑：
    // - 如果 version < 2 且 partitionCount == 0，则不解析任何分区（合法）
    // - 如果 partitionCount < 0（理论上不会发生，但保留判断），也跳过
    if ((partitionCount == 0 && version < 2) || partitionCount <= 0)
    {
        m_partitions.clear();
        return ErrNoError;
    }

    m_partitions.clear();
    m_partitions.reserve(partitionCount);

    for (int32_t i = 0; i < partitionCount; ++i)
    {
        std::string topic;
        if (pd.getString(topic) != ErrNoError)
        {
            return ErrDecodeError;
        }

        std::vector<int32_t> partitionsList;
        if (pd.getInt32Array(partitionsList) != ErrNoError)
        {
            return ErrDecodeError;
        }

        m_partitions[topic] = std::move(partitionsList);
    }

    return ErrNoError;
}

int16_t DeleteOffsetsRequest::key() const
{
    return apiKeyOffsetDelete;
}

int16_t DeleteOffsetsRequest::version() const
{
    return m_version;
}

int16_t DeleteOffsetsRequest::header_version() const
{
    return 1;
}

bool DeleteOffsetsRequest::is_valid_version() const
{
    return m_version == 0;
}

KafkaVersion DeleteOffsetsRequest::required_version() const
{
    return V2_4_0_0;
}

void DeleteOffsetsRequest::AddPartition(const std::string &topic, int32_t partitionID)
{
    m_partitions[topic].push_back(partitionID);
}