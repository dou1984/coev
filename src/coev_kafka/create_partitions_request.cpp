#include "create_partitions_request.h"
#include "api_versions.h"
void CreatePartitionsRequest::setVersion(int16_t v)
{
    Version = v;
}

int CreatePartitionsRequest::encode(PEncoder &pe)
{
    if (!pe.putArrayLength(static_cast<int32_t>(TopicPartitions.size())))
    {
        return ErrEncodeError;
    }

    for (auto &pair : TopicPartitions)
    {
        if (!pe.putString(pair.first))
        {
            return ErrEncodeError;
        }
        if (!pair.second->encode(pe))
        {
            return ErrEncodeError;
        }
    }

    pe.putInt32(static_cast<int32_t>(Timeout.count()));
    pe.putBool(ValidateOnly);

    return true;
}

int CreatePartitionsRequest::decode(PDecoder &pd, int16_t version)
{
    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    TopicPartitions.clear();
    for (int32_t i = 0; i < n; ++i)
    {
        std::string topic;
        if (pd.getString(topic) != ErrNoError)
        {
            return ErrDecodeError;
        }
        auto partition = std::make_shared<TopicPartition>();
        if (partition->decode(pd, version) != ErrNoError)
        {
            return ErrDecodeError;
        }
        TopicPartitions[topic] = partition;
    }

    if (pd.getDurationMs(Timeout) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (pd.getBool(ValidateOnly) != ErrNoError)
    {
        return ErrDecodeError;
    }

    return ErrNoError;
}

int16_t CreatePartitionsRequest::key() const
{
    return apiKeyCreatePartitions;
}

int16_t CreatePartitionsRequest::version() const
{
    return Version;
}

int16_t CreatePartitionsRequest::headerVersion() const
{
    return 1;
}

bool CreatePartitionsRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 1;
}

KafkaVersion CreatePartitionsRequest::requiredVersion() const
{
    switch (Version)
    {
    case 1:
        return V2_0_0_0;
    case 0:
        return V1_0_0_0;
    default:
        return V2_0_0_0;
    }
}

std::chrono::milliseconds CreatePartitionsRequest::throttleTime() const
{
    return Timeout;
}