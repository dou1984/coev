#include "version.h"
#include "describe_log_dirs_request.h"

void DescribeLogDirsRequest::setVersion(int16_t v)
{
    Version = v;
}

int DescribeLogDirsRequest::encode(PEncoder &pe)
{
    int32_t length = static_cast<int32_t>(DescribeTopics.size());
    if (length == 0)
    {
        length = -1;
    }
    if (!pe.putArrayLength(length))
    {
        return ErrEncodeError;
    }

    for (auto &d : DescribeTopics)
    {
        if (!pe.putString(d.Topic))
        {
            return ErrEncodeError;
        }
        if (!pe.putInt32Array(d.PartitionIDs))
        {
            return ErrEncodeError;
        }
        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return true;
}

int DescribeLogDirsRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    int32_t n;
    if (!pd.getArrayLength(n))
    {
        return ErrDecodeError;
    }

    if (n == -1)
    {
        n = 0;
    }

    std::vector<DescribeLogDirsRequestTopic> topics(n);
    for (int32_t i = 0; i < n; ++i)
    {
        std::string topic;
        if (!pd.getString(topic))
        {
            return ErrDecodeError;
        }
        topics[i].Topic = topic;

        if (!pd.getInt32Array(topics[i].PartitionIDs))
        {
            return ErrDecodeError;
        }

        int32_t dummy;
        if (!pd.getEmptyTaggedFieldArray(dummy))
        {
            return ErrDecodeError;
        }
    }
    DescribeTopics = std::move(topics);

    int32_t dummy;
    if (!pd.getEmptyTaggedFieldArray(dummy))
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int16_t DescribeLogDirsRequest::key() const
{
    return apiKeyDescribeLogDirs;
}

int16_t DescribeLogDirsRequest::version() const
{
    return Version;
}

int16_t DescribeLogDirsRequest::headerVersion() const
{
    if (Version >= 2)
    {
        return 2;
    }
    return 1;
}

bool DescribeLogDirsRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 4;
}

bool DescribeLogDirsRequest::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool DescribeLogDirsRequest::isFlexibleVersion(int16_t version)
{
    return version >= 2;
}

KafkaVersion DescribeLogDirsRequest::requiredVersion() const
{
    switch (Version)
    {
    case 4:
        return V3_3_0_0;
    case 3:
        return V3_2_0_0;
    case 2:
        return V2_6_0_0;
    case 1:
        return V2_0_0_0;
    default:
        return V1_0_0_0;
    }
}