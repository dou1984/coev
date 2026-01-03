#include "version.h"
#include "delete_records_response.h"
#include "api_versions.h"
#include <algorithm>

int DeleteRecordsResponsePartition::encode(PEncoder &pe)
{
    pe.putInt64(LowWatermark);
    pe.putInt16(static_cast<int16_t>(Err));
    return true;
}

int DeleteRecordsResponsePartition::decode(PDecoder &pd, int16_t /*version*/)
{
    if (pd.getInt64(LowWatermark) != ErrNoError)
    {
        return ErrEncodeError;
    }

    int16_t errCode;
    if (pd.getInt16(errCode) != ErrNoError)
    {
        return ErrEncodeError;
    }
    Err = static_cast<KError>(errCode);

    return true;
}
int DeleteRecordsResponseTopic::encode(PEncoder &pe)
{
    if (!pe.putArrayLength(static_cast<int32_t>(Partitions.size())))
    {
        return ErrEncodeError;
    }

    // Sort partition IDs for deterministic encoding
    std::vector<int32_t> keys;
    keys.reserve(Partitions.size());
    for (auto &kv : Partitions)
    {
        keys.push_back(kv.first);
    }
    std::sort(keys.begin(), keys.end());

    for (int32_t partition : keys)
    {
        pe.putInt32(partition);
        if (!Partitions.at(partition)->encode(pe))
        {
            return ErrEncodeError;
        }
    }

    return true;
}

int DeleteRecordsResponseTopic::decode(PDecoder &pd, int16_t version)
{
    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrEncodeError;
    }

    Partitions.clear();
    if (n > 0)
    {
        for (int32_t i = 0; i < n; ++i)
        {
            int32_t partition;
            if (pd.getInt32(partition) != ErrNoError)
            {
                return ErrEncodeError;
            }

            auto details = std::shared_ptr<DeleteRecordsResponsePartition>();
            if (!details->decode(pd, version))
            {
                return ErrEncodeError;
            }
            Partitions[partition] = details;
        }
    }

    return ErrNoError;
}

DeleteRecordsResponseTopic::~DeleteRecordsResponseTopic()
{
    Partitions.clear();
}

void DeleteRecordsResponse::setVersion(int16_t v)
{
    Version = v;
}

int DeleteRecordsResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(ThrottleTime);

    if (!pe.putArrayLength(static_cast<int32_t>(Topics.size())))
    {
        return ErrEncodeError;
    }

    // Sort topic names for deterministic encoding
    std::vector<std::string> keys;
    keys.reserve(Topics.size());
    for (auto &kv : Topics)
    {
        keys.push_back(kv.first);
    }
    std::sort(keys.begin(), keys.end());

    for (const std::string &topic : keys)
    {
        if (!pe.putString(topic))
        {
            return ErrEncodeError;
        }
        if (!Topics.at(topic)->encode(pe))
        {
            return ErrEncodeError;
        }
    }

    return ErrNoError;
}

int DeleteRecordsResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    if (pd.getDurationMs(ThrottleTime) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    Topics.clear();

    if (n > 0)
    {
        for (int32_t i = 0; i < n; ++i)
        {
            std::string topic;
            if (pd.getString(topic) != ErrNoError)
            {
                return ErrDecodeError;
            }

            auto details = std::make_shared<DeleteRecordsResponseTopic>();
            if (!details->decode(pd, version))
            {
                return ErrDecodeError;
            }
            Topics[topic] = details;
        }
    }

    return ErrNoError;
}

int16_t DeleteRecordsResponse::key() const
{
    return apiKeyDeleteRecords;
}

int16_t DeleteRecordsResponse::version() const
{
    return Version;
}

int16_t DeleteRecordsResponse::headerVersion() const
{
    return 0; // non-flexible format
}

bool DeleteRecordsResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 1;
}

KafkaVersion DeleteRecordsResponse::requiredVersion() const
{
    switch (Version)
    {
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}

std::chrono::milliseconds DeleteRecordsResponse::throttleTime() const
{
    return ThrottleTime;
}

DeleteRecordsResponse::~DeleteRecordsResponse()
{

    Topics.clear();
}