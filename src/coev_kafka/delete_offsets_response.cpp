#include "version.h"
#include "delete_offsets_response.h"

void DeleteOffsetsResponse::setVersion(int16_t v)
{
    Version = v;
}

void DeleteOffsetsResponse::AddError(const std::string &topic, int32_t partition, KError errorCode)
{
    auto &partitions = Errors[topic];
    partitions[partition] = errorCode;
}

int DeleteOffsetsResponse::encode(PEncoder &pe)
{
    pe.putInt16(static_cast<int16_t>(ErrorCode));
    pe.putDurationMs(ThrottleTime);

    if (!pe.putArrayLength(static_cast<int32_t>(Errors.size())))
    {
        return ErrEncodeError;
    }

    for (auto &topicEntry : Errors)
    {
        const std::string &topic = topicEntry.first;
        auto &partitions = topicEntry.second;

        if (!pe.putString(topic))
        {
            return ErrEncodeError;
        }
        if (!pe.putArrayLength(static_cast<int32_t>(partitions.size())))
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

    return true;
}

int DeleteOffsetsResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    int16_t errCode;
    if (pd.getInt16(errCode) != ErrNoError)
    {
        return ErrDecodeError;
    }
    ErrorCode = static_cast<KError>(errCode);

    if (pd.getDurationMs(ThrottleTime) != ErrNoError)
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
        Errors.clear();
        return true;
    }

    Errors.clear();
    Errors.reserve(numTopics);

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

        auto &partitionMap = Errors[topic];
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

    return true;
}

int16_t DeleteOffsetsResponse::key() const
{
    return 47; // apiKeyOffsetDelete
}

int16_t DeleteOffsetsResponse::version() const
{
    return Version;
}

int16_t DeleteOffsetsResponse::headerVersion() const
{
    return 0;
}

bool DeleteOffsetsResponse::isValidVersion() const
{
    return Version == 0;
}

KafkaVersion DeleteOffsetsResponse::requiredVersion() const
{
    return V2_4_0_0;
}

std::chrono::milliseconds DeleteOffsetsResponse::throttleTime() const
{
    return ThrottleTime;
}