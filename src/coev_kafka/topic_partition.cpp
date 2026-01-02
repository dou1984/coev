#include "topic_partition.h"

TopicPartition::TopicPartition(const std::string &t, int32_t p) : topic(t), partition(p)
{
}
int TopicPartition::encode(PEncoder &pe)
{
    pe.putInt32(Count);

    if (Assignment.empty())
    {
        pe.putInt32(-1);
        return true;
    }

    if (!pe.putArrayLength(static_cast<int32_t>(Assignment.size())))
    {
        return ErrEncodeError;
    }

    for (auto &assign : Assignment)
    {
        if (!pe.putInt32Array(assign))
        {
            return ErrEncodeError;
        }
    }

    return true;
}

int TopicPartition::decode(PDecoder &pd, int16_t version)
{
    if (!pd.getInt32(Count))
    {
        return ErrDecodeError;
    }

    int32_t n;
    if (!pd.getInt32(n))
    {
        return ErrDecodeError;
    }

    if (n <= 0)
    {
        return true;
    }

    Assignment.resize(static_cast<size_t>(n));
    for (int32_t i = 0; i < n; ++i)
    {
        if (!pd.getInt32Array(Assignment[i]))
        {
            return ErrDecodeError;
        }
    }

    return ErrNoError;
}