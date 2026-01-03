#include <algorithm>
#include "version.h"
#include "delete_records_request.h"
#include "api_versions.h"

int DeleteRecordsRequestTopic::encode(PEncoder &pe)
{
    if (!pe.putArrayLength(static_cast<int32_t>(PartitionOffsets.size())))
    {
        return ErrEncodeError;
    }

    std::vector<int32_t> keys;
    keys.reserve(PartitionOffsets.size());
    for (auto &kv : PartitionOffsets)
    {
        keys.push_back(kv.first);
    }
    std::sort(keys.begin(), keys.end());

    for (int32_t partition : keys)
    {
        pe.putInt32(partition);
        pe.putInt64(PartitionOffsets.at(partition));
    }

    return true;
}

int DeleteRecordsRequestTopic::decode(PDecoder &pd, int16_t /*version*/)
{
    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrEncodeError;
    }

    PartitionOffsets.clear();

    if (n > 0)
    {
        PartitionOffsets.reserve(n);
        for (int32_t i = 0; i < n; ++i)
        {
            int32_t partition;
            int64_t offset;
            if (pd.getInt32(partition) || !pd.getInt64(offset) != ErrNoError)
            {
                return ErrEncodeError;
            }
            PartitionOffsets[partition] = offset;
        }
    }

    return true;
}

void DeleteRecordsRequest::setVersion(int16_t v)
{
    Version = v;
}

int DeleteRecordsRequest::encode(PEncoder &pe)
{
    if (!pe.putArrayLength(static_cast<int32_t>(Topics.size())))
    {
        return ErrEncodeError;
    }

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

    pe.putInt32(static_cast<int32_t>(Timeout.count()));
    return true;
}

int DeleteRecordsRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;

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

            auto details = std::make_shared<DeleteRecordsRequestTopic>();
            if (!details->decode(pd, version))
            {

                return ErrDecodeError;
            }
            Topics[topic] = details;
        }
    }

    if (pd.getDurationMs(Timeout) != ErrNoError)
    {
        return ErrDecodeError;
    }

    return ErrNoError;
}

int16_t DeleteRecordsRequest::key() const
{
    return apiKeyDeleteRecords;
}

int16_t DeleteRecordsRequest::version() const
{
    return Version;
}

int16_t DeleteRecordsRequest::headerVersion() const
{
    return 1;
}

bool DeleteRecordsRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 1;
}

KafkaVersion DeleteRecordsRequest::requiredVersion() const
{
    switch (Version)
    {
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0; // V0
    }
}

DeleteRecordsRequest::~DeleteRecordsRequest()
{

    Topics.clear();
}