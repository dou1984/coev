#include "topic_partition.h"

TopicPartition::TopicPartition(const std::string &t, int32_t p) : topic_t(t, p)
{
}

bool TopicPartition::operator==(const TopicPartition &other) const
{
    return m_topic == other.m_topic && m_partition == other.m_partition;
}
int TopicPartition::encode(packet_encoder &pe)
{
    pe.putInt32(m_count);

    if (m_assignment.empty())
    {
        pe.putInt32(-1);
        return ErrNoError;
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_assignment.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &assign : m_assignment)
    {
        if (pe.putInt32Array(assign) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    return ErrNoError;
}

int TopicPartition::decode(packet_decoder &pd, int16_t version)
{
    if (pd.getInt32(m_count) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t n;
    if (pd.getInt32(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (n <= 0)
    {
        return ErrNoError;
    }

    m_assignment.resize(static_cast<size_t>(n));
    for (int32_t i = 0; i < n; ++i)
    {
        if (pd.getInt32Array(m_assignment[i]) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    return ErrNoError;
}