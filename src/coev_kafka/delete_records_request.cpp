#include <algorithm>
#include "version.h"
#include "delete_records_request.h"
#include "api_versions.h"

int DeleteRecordsRequestTopic::encode(PEncoder &pe)
{
    if (pe.putArrayLength(static_cast<int32_t>(m_partition_offsets.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    std::vector<int32_t> keys;
    keys.reserve(m_partition_offsets.size());
    for (auto &kv : m_partition_offsets)
    {
        keys.push_back(kv.first);
    }
    std::sort(keys.begin(), keys.end());

    for (int32_t partition : keys)
    {
        pe.putInt32(partition);
        pe.putInt64(m_partition_offsets.at(partition));
    }

    return ErrNoError;
}

int DeleteRecordsRequestTopic::decode(PDecoder &pd, int16_t /*version*/)
{
    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrEncodeError;
    }

    m_partition_offsets.clear();

    if (n > 0)
    {
        m_partition_offsets.reserve(n);
        for (int32_t i = 0; i < n; ++i)
        {
            int32_t partition;
            int64_t offset;
            if (pd.getInt32(partition) != ErrNoError || pd.getInt64(offset) != ErrNoError)
            {
                return ErrEncodeError;
            }
            m_partition_offsets[partition] = offset;
        }
    }

    return ErrNoError;
}

void DeleteRecordsRequest::set_version(int16_t v)
{
    m_version = v;
}

int DeleteRecordsRequest::encode(PEncoder &pe)
{
    if (pe.putArrayLength(static_cast<int32_t>(m_topics.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    std::vector<std::string> keys;
    keys.reserve(m_topics.size());
    for (auto &kv : m_topics)
    {
        keys.push_back(kv.first);
    }
    std::sort(keys.begin(), keys.end());

    for (const std::string &topic : keys)
    {
        if (pe.putString(topic) != ErrNoError)
        {
            return ErrEncodeError;
        }
        if (m_topics.at(topic)->encode(pe) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    pe.putInt32(static_cast<int32_t>(m_timeout.count()));
    return ErrNoError;
}

int DeleteRecordsRequest::decode(PDecoder &pd, int16_t version)
{
    m_version = version;

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_topics.clear();

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
            if (details->decode(pd, version) != ErrNoError)
            {
                return ErrDecodeError;
            }
            m_topics[topic] = details;
        }
    }

    if (pd.getDurationMs(m_timeout) != ErrNoError)
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
    return m_version;
}

int16_t DeleteRecordsRequest::headerVersion() const
{
    return 1;
}

bool DeleteRecordsRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

KafkaVersion DeleteRecordsRequest::required_version() const
{
    switch (m_version)
    {
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0; // V0
    }
}

DeleteRecordsRequest::~DeleteRecordsRequest()
{

    m_topics.clear();
}