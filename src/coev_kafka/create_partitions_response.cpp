#include "create_partitions_response.h"
#include <sstream>

void CreatePartitionsResponse::setVersion(int16_t v)
{
    Version = v;
}

int CreatePartitionsResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(ThrottleTime);

    if (!pe.putArrayLength(static_cast<int32_t>(TopicPartitionErrors.size())))
    {
        return ErrEncodeError;
    }

    for (auto &pair : TopicPartitionErrors)
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

    return true;
}

int CreatePartitionsResponse::decode(PDecoder &pd, int16_t version)
{
    if (pd.getDurationMs(ThrottleTime) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    TopicPartitionErrors.clear();
    for (int32_t i = 0; i < n; ++i)
    {
        std::string topic;
        if (pd.getString(topic) != ErrNoError)
        {
            return ErrDecodeError;
        }
        auto err = std::make_shared<TopicPartitionError>();
        if (!err->decode(pd, version))
        {
            return ErrDecodeError;
        }
        TopicPartitionErrors[topic] = err;
    }

    return ErrNoError;
}

int16_t CreatePartitionsResponse::key()const{
    return 37; // apiKeyCreatePartitions
}

int16_t CreatePartitionsResponse::version()const
{
    return Version;
}

int16_t CreatePartitionsResponse::headerVersion()const
{
    return 0;
}

bool CreatePartitionsResponse::isValidVersion()const
{
    return Version >= 0 && Version <= 1;
}

KafkaVersion CreatePartitionsResponse::requiredVersion() const
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

std::chrono::milliseconds CreatePartitionsResponse::throttleTime() const
{
    return ThrottleTime;
}

std::string TopicPartitionError::Error() const
{
    std::string text = ""; // In real code, this would map KError to string
    if (!ErrMsg.empty())
    {
        text += " - " + ErrMsg;
    }
    return text;
}

int TopicPartitionError::encode(PEncoder &pe)
{
    pe.putKError(Err);
    if (!pe.putNullableString(ErrMsg))
    {
        return ErrEncodeError;
    }
    return true;
}

int TopicPartitionError::decode(PDecoder &pd, int16_t version)
{
    if (pd.getKError(Err) != ErrNoError)
    {
        return ErrEncodeError;
    }

    if (pd.getNullableString(ErrMsg) != ErrNoError)
    {
        return ErrEncodeError;
    }

    return ErrNoError;
}