#include "version.h"
#include "delete_offsets_request.h"

void DeleteOffsetsRequest::set_version(int16_t v)
{
    m_version = v;
}

int DeleteOffsetsRequest::encode(packet_encoder &pe) const
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
        for (const auto &kv : m_partitions)
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

int DeleteOffsetsRequest::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;

    if (pd.getString(m_group) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t partition_count;
    if (pd.getArrayLength(partition_count) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if ((partition_count == 0 && version < 2) || partition_count <= 0)
    {
        m_partitions.clear();
        return ErrNoError;
    }

    m_partitions.clear();
    m_partitions.reserve(partition_count);

    for (int32_t i = 0; i < partition_count; ++i)
    {
        std::string topic;
        if (pd.getString(topic) != ErrNoError)
        {
            return ErrDecodeError;
        }

        auto &partitions_list = m_partitions[topic];
        if (pd.getInt32Array(partitions_list) != ErrNoError)
        {
            return ErrDecodeError;
        }
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
    return m_version >= 0 && m_version <= 2;
}

KafkaVersion DeleteOffsetsRequest::required_version() const
{
    return V2_4_0_0;
}

void DeleteOffsetsRequest::add_partition(const std::string &topic, int32_t partitionID)
{
    m_partitions[topic].push_back(partitionID);
}