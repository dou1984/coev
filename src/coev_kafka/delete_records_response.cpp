#include "version.h"
#include "delete_records_response.h"
#include "api_versions.h"
#include <algorithm>

int DeleteRecordsResponsePartition::encode(packet_encoder &pe) const
{
    pe.putInt64(m_low_watermark);
    pe.putInt16(static_cast<int16_t>(m_err));
    return ErrNoError;
}

int DeleteRecordsResponsePartition::decode(packet_decoder &pd, int16_t /*version*/)
{
    if (pd.getInt64(m_low_watermark) != ErrNoError)
    {
        return ErrEncodeError;
    }

    int16_t errCode;
    if (pd.getInt16(errCode) != ErrNoError)
    {
        return ErrEncodeError;
    }
    m_err = static_cast<KError>(errCode);

    return ErrNoError;
}
int DeleteRecordsResponseTopic::encode(packet_encoder &pe) const
{
    if (pe.putArrayLength(static_cast<int32_t>(m_partitions.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    // Sort partition IDs for deterministic encoding
    std::vector<int32_t> keys;
    keys.reserve(m_partitions.size());
    for (auto &kv : m_partitions)
    {
        keys.push_back(kv.first);
    }
    std::sort(keys.begin(), keys.end());

    for (int32_t partition : keys)
    {
        pe.putInt32(partition);
        if (m_partitions.at(partition)->encode(pe) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    return ErrNoError;
}

int DeleteRecordsResponseTopic::decode(packet_decoder &pd, int16_t version)
{
    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrEncodeError;
    }

    m_partitions.clear();
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
            if (details->decode(pd, version) != ErrNoError)
            {
                return ErrEncodeError;
            }
            m_partitions[partition] = details;
        }
    }

    return ErrNoError;
}

DeleteRecordsResponseTopic::~DeleteRecordsResponseTopic()
{
    m_partitions.clear();
}

void DeleteRecordsResponse::set_version(int16_t v)
{
    m_version = v;
}

int DeleteRecordsResponse::encode(packet_encoder &pe) const
{
    pe.putDurationMs(m_throttle_time);

    if (pe.putArrayLength(static_cast<int32_t>(m_topics.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    // Sort topic names for deterministic encoding
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

    return ErrNoError;
}

int DeleteRecordsResponse::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;

    if (pd.getDurationMs(m_throttle_time) != ErrNoError)
    {
        return ErrDecodeError;
    }

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

            auto details = std::make_shared<DeleteRecordsResponseTopic>();
            if (details->decode(pd, version) != ErrNoError)
            {
                return ErrDecodeError;
            }
            m_topics[topic] = details;
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
    return m_version;
}

int16_t DeleteRecordsResponse::header_version() const
{
    return 0; // non-flexible format
}

bool DeleteRecordsResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 2;
}

KafkaVersion DeleteRecordsResponse::required_version() const
{
    switch (m_version)
    {
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}

std::chrono::milliseconds DeleteRecordsResponse::throttle_time() const
{
    return m_throttle_time;
}

DeleteRecordsResponse::~DeleteRecordsResponse()
{

    m_topics.clear();
}