#include "elect_leaders_request.h"
#include <vector>

void ElectLeadersRequest::setVersion(int16_t v)
{
    Version = v;
}

int ElectLeadersRequest::encode(PEncoder &pe)
{
    if (Version > 0)
    {
        pe.putInt8(static_cast<int8_t>(Type));
    }

    if (!pe.putArrayLength(static_cast<int32_t>(TopicPartitions.size())))
    {
        return ErrEncodeError;
    }

    for (auto &kv : TopicPartitions)
    {
        if (!pe.putString(kv.first))
        {
            return ErrEncodeError;
        }
        if (!pe.putInt32Array(kv.second))
        {
            return ErrEncodeError;
        }
        pe.putEmptyTaggedFieldArray();
    }

    pe.putDurationMs(Timeout);
    pe.putEmptyTaggedFieldArray();
    return true;
}

int ElectLeadersRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    if (Version > 0)
    {
        int8_t t;
        if (!pd.getInt8(t))
        {
            return ErrDecodeError;
        }
        Type = static_cast<ElectionType>(t);
    }

    int32_t topicCount;
    if (!pd.getArrayLength(topicCount))
    {
        return ErrDecodeError;
    }

    TopicPartitions.clear();
    if (topicCount > 0)
    {
        for (int32_t i = 0; i < topicCount; ++i)
        {
            std::string topic;
            if (!pd.getString(topic))
            {
                return ErrDecodeError;
            }

            int32_t partitionCount;
            if (!pd.getArrayLength(partitionCount))
            {
                return ErrDecodeError;
            }

            std::vector<int32_t> partitions(partitionCount);
            for (int32_t j = 0; j < partitionCount; ++j)
            {
                if (!pd.getInt32(partitions[j]))
                {
                    return ErrDecodeError;
                }
            }

            TopicPartitions[topic] = std::move(partitions);

            int32_t dummy;
            if (!pd.getEmptyTaggedFieldArray(dummy))
            {
                return ErrDecodeError;
            }
        }
    }

    if (!pd.getDurationMs(Timeout))
    {
        return ErrDecodeError;
    }

    int32_t dummy;
    if (!pd.getEmptyTaggedFieldArray(dummy))
    {
        return ErrDecodeError;
    }

    return ErrNoError;
}

int16_t ElectLeadersRequest::key() const
{
    return apiKeyElectLeaders;
}

int16_t ElectLeadersRequest::version() const
{
    return Version;
}

int16_t ElectLeadersRequest::headerVersion() const
{
    return 2;
}

bool ElectLeadersRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 2;
}

bool ElectLeadersRequest::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool ElectLeadersRequest::isFlexibleVersion(int16_t version)
{
    return version >= 2;
}

KafkaVersion ElectLeadersRequest::requiredVersion() const
{
    switch (Version)
    {
    case 2:
        return V2_4_0_0;
    case 1:
        return V0_11_0_0;
    case 0:
        return V0_10_0_0;
    default:
        return V2_4_0_0;
    }
}