#include "version.h"
#include "delete_offsets_request.h"

void DeleteOffsetsRequest::setVersion(int16_t v)
{
    Version = v;
}

int DeleteOffsetsRequest::encode(PEncoder &pe)
{
    if (!pe.putString(Group))
    {
        return ErrEncodeError;
    }

    if (partitions.empty())
    {
        pe.putInt32(0); // array length = 0
    }
    else
    {
        if (!pe.putArrayLength(static_cast<int32_t>(partitions.size())))
        {
            return ErrEncodeError;
        }
        for (auto &kv : partitions)
        {
            if (!pe.putString(kv.first))
            {
                return ErrEncodeError;
            }
            if (!pe.putInt32Array(kv.second))
            {
                return ErrEncodeError;
            }
        }
    }

    return true;
}

int DeleteOffsetsRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    if (!pd.getString(Group))
    {
        return ErrDecodeError;
    }

    int32_t partitionCount;
    if (!pd.getArrayLength(partitionCount))
    {
        return ErrDecodeError;
    }

    // 根据 Go 逻辑：
    // - 如果 version < 2 且 partitionCount == 0，则不解析任何分区（合法）
    // - 如果 partitionCount < 0（理论上不会发生，但保留判断），也跳过
    if ((partitionCount == 0 && version < 2) || partitionCount <= 0)
    {
        partitions.clear();
        return ErrNoError;
    }

    partitions.clear();
    partitions.reserve(partitionCount);

    for (int32_t i = 0; i < partitionCount; ++i)
    {
        std::string topic;
        if (!pd.getString(topic))
        {
            return ErrDecodeError;
        }

        std::vector<int32_t> partitionsList;
        if (!pd.getInt32Array(partitionsList))
        {
            return ErrDecodeError;
        }

        partitions[topic] = std::move(partitionsList);
    }

    return ErrNoError;
}

int16_t DeleteOffsetsRequest::key() const
{
    return apiKeyOffsetDelete;
}

int16_t DeleteOffsetsRequest::version() const
{
    return Version;
}

int16_t DeleteOffsetsRequest::headerVersion() const
{
    return 1;
}

bool DeleteOffsetsRequest::isValidVersion() const
{
    return Version == 0;
}

KafkaVersion DeleteOffsetsRequest::requiredVersion() const
{
    return V2_4_0_0;
}

void DeleteOffsetsRequest::AddPartition(const std::string &topic, int32_t partitionID)
{
    partitions[topic].push_back(partitionID);
}