#include "version.h"
#include "delete_topics_response.h"

void DeleteTopicsResponse::setVersion(int16_t v)
{
    Version = v;
}

int DeleteTopicsResponse::encode(PEncoder &pe)
{
    if (Version >= 1)
    {
        pe.putDurationMs(ThrottleTime);
    }

    if (!pe.putArrayLength(static_cast<int32_t>(TopicErrorCodes.size())))
    {
        return ErrEncodeError;
    }
    for (auto &kv : TopicErrorCodes)
    {
        if (!pe.putString(kv.first))
        {
            return ErrEncodeError;
        }
        pe.putInt16(static_cast<int16_t>(kv.second));
        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return true;
}

int DeleteTopicsResponse::decode(PDecoder &pd, int16_t version)
{
    if (version >= 1)
    {

        if (pd.getDurationMs(ThrottleTime) != ErrNoError)
        {
            return ErrDecodeError;
        }

        Version = version;
    }

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    TopicErrorCodes.clear();

    for (int32_t i = 0; i < n; ++i)
    {
        std::string topic;
        if (pd.getString(topic) != ErrNoError)
        {
            return ErrDecodeError;
        }
        int16_t errCode;
        if (pd.getInt16(errCode) != ErrNoError)
        {
            return ErrDecodeError;
        }
        TopicErrorCodes[topic] = static_cast<KError>(errCode);

        int32_t _;
        if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }
    int32_t _;
    if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int16_t DeleteTopicsResponse::key() const
{
    return apiKeyDeleteTopics;
}

int16_t DeleteTopicsResponse::version() const
{
    return Version;
}

int16_t DeleteTopicsResponse::headerVersion() const
{
    if (Version >= 4)
    {
        return 1;
    }
    return 0;
}

bool DeleteTopicsResponse::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool DeleteTopicsResponse::isFlexibleVersion(int16_t version)
{
    return version >= 4;
}

bool DeleteTopicsResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 4;
}

KafkaVersion DeleteTopicsResponse::requiredVersion() const
{
    switch (Version)
    {
    case 4:
        return V2_4_0_0;
    case 3:
        return V2_1_0_0;
    case 2:
        return V2_0_0_0;
    case 1:
        return V0_11_0_0;
    case 0:
        return V0_10_1_0;
    default:
        return V2_2_0_0;
    }
}

std::chrono::milliseconds DeleteTopicsResponse::throttleTime() const
{
    return ThrottleTime;
}