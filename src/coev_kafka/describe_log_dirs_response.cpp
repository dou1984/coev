#include "version.h"
#include "api_versions.h"
#include "describe_log_dirs_response.h"

void DescribeLogDirsResponse::setVersion(int16_t v)
{
    Version = v;
}

int DescribeLogDirsResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(ThrottleTime);
    if (Version >= 3)
    {
        pe.putKError(ErrorCode);
    }

    if (!pe.putArrayLength(static_cast<int32_t>(LogDirs.size())))
    {
        return ErrEncodeError;
    }

    for (auto &dir : LogDirs)
    {
        if (!dir.encode(pe, Version))
        {
            return ErrEncodeError;
        }
    }

    pe.putEmptyTaggedFieldArray();
    return true;
}

int DescribeLogDirsResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    if (pd.getDurationMs(ThrottleTime) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (version >= 3)
    {
        if (pd.getKError(ErrorCode) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    LogDirs.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        if (!LogDirs[i].decode(pd, version))
        {
            return ErrDecodeError;
        }
    }

    int32_t dummy;
    if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int16_t DescribeLogDirsResponse::key() const
{
    return apiKeyDescribeLogDirs;
}

int16_t DescribeLogDirsResponse::version() const
{
    return Version;
}

int16_t DescribeLogDirsResponse::headerVersion() const
{
    if (Version >= 2)
    {
        return 1;
    }
    return 0;
}

bool DescribeLogDirsResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 4;
}

bool DescribeLogDirsResponse::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool DescribeLogDirsResponse::isFlexibleVersion(int16_t version)
{
    return version >= 2;
}

KafkaVersion DescribeLogDirsResponse::requiredVersion() const
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

std::chrono::milliseconds DescribeLogDirsResponse::throttleTime() const
{
    return ThrottleTime;
}

int DescribeLogDirsResponseDirMetadata::encode(PEncoder &pe, int16_t version)
{
    pe.putKError(ErrorCode);

    if (!pe.putString(Path))
    {
        return ErrEncodeError;
    }

    if (!pe.putArrayLength(static_cast<int32_t>(Topics.size())))
    {
        return ErrEncodeError;
    }

    for (auto &topic : Topics)
    {
        if (!topic.encode(pe, version))
        {
            return ErrEncodeError;
        }
    }

    if (version >= 4)
    {
        pe.putInt64(TotalBytes);
        pe.putInt64(UsableBytes);
    }

    pe.putEmptyTaggedFieldArray();
    return true;
}

int DescribeLogDirsResponseDirMetadata::decode(PDecoder &pd, int16_t version)
{
    if (pd.getKError(ErrorCode) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (pd.getString(Path) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    Topics.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        if (!Topics[i].decode(pd, version))
        {
            return ErrDecodeError;
        }
    }

    if (version >= 4)
    {
        if (pd.getInt64(TotalBytes) != ErrNoError)
            return ErrDecodeError;
        if (pd.getInt64(UsableBytes) != ErrNoError)
            return ErrDecodeError;
    }

    int32_t dummy;
    if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int DescribeLogDirsResponseTopic::encode(PEncoder &pe, int16_t version)
{
    if (!pe.putString(Topic))
    {
        return ErrEncodeError;
    }

    if (!pe.putArrayLength(static_cast<int32_t>(Partitions.size())))
    {
        return ErrEncodeError;
    }

    for (auto &partition : Partitions)
    {
        if (!partition.encode(pe, version))
        {
            return ErrEncodeError;
        }
    }

    pe.putEmptyTaggedFieldArray();
    return true;
}

int DescribeLogDirsResponseTopic::decode(PDecoder &pd, int16_t version)
{
    if (pd.getString(Topic) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    Partitions.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        if (!Partitions[i].decode(pd, version))
        {
            return ErrDecodeError;
        }
    }

    int32_t dummy;
    if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int DescribeLogDirsResponsePartition::encode(PEncoder &pe, int16_t version)
{
    pe.putInt32(PartitionID);
    pe.putInt64(Size);
    pe.putInt64(OffsetLag);
    pe.putBool(IsTemporary);

    if (version >= 2)
    {
        pe.putEmptyTaggedFieldArray();
    }

    return true;
}

int DescribeLogDirsResponsePartition::decode(PDecoder &pd, int16_t version)
{
    if (pd.getInt32(PartitionID) != ErrNoError)
        return ErrDecodeError;
    if (pd.getInt64(Size) != ErrNoError)
        return ErrDecodeError;
    if (pd.getInt64(OffsetLag) != ErrNoError)
        return ErrDecodeError;
    if (pd.getBool(IsTemporary) != ErrNoError)
        return ErrDecodeError;

    int32_t dummy;
    if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}