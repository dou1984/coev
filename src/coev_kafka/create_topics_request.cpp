#include "api_versions.h"
#include "create_topics_request.h"

void CreateTopicsRequest::set_version(int16_t v)
{
    m_version = v;
}

int CreateTopicsRequest::encode(PEncoder &pe)
{
    if (pe.putArrayLength(static_cast<int32_t>(m_topic_details.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &kv : m_topic_details)
    {
        if (pe.putString(kv.first) != ErrNoError)
        {
            return ErrEncodeError;
        }
        if (kv.second->encode(pe) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    pe.putDurationMs(m_timeout);

    if (m_version >= 1)
    {
        pe.putBool(m_validate_only);
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int CreateTopicsRequest::decode(PDecoder &pd, int16_t version)
{
    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_topic_details.clear();
    for (int32_t i = 0; i < n; ++i)
    {
        std::string topic;
        if (pd.getString(topic) != ErrNoError)
        {
            return ErrDecodeError;
        }
        auto detail = std::make_shared<TopicDetail>();
        if (detail->decode(pd, version) != ErrNoError)
        {
            return ErrDecodeError;
        }
        m_topic_details[topic] = detail;
    }

    if (pd.getDurationMs(m_timeout) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (version >= 1)
    {
        if (pd.getBool(m_validate_only) != ErrNoError)
        {
            return ErrDecodeError;
        }
        m_version = version;
    }

    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t CreateTopicsRequest::key() const
{
    return apiKeyCreateTopics;
}

int16_t CreateTopicsRequest::version() const
{
    return m_version;
}

int16_t CreateTopicsRequest::headerVersion() const
{
    return m_version >= 5 ? 2 : 1;
}

bool CreateTopicsRequest::isFlexible() const
{
    return isFlexibleVersion(m_version);
}

bool CreateTopicsRequest::isFlexibleVersion(int16_t version)
{
    return version >= 5;
}

bool CreateTopicsRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 5;
}

KafkaVersion CreateTopicsRequest::required_version() const
{
    switch (m_version)
    {
    case 5:
        return V2_4_0_0;
    case 4:
        return V2_4_0_0;
    case 3:
        return V2_0_0_0;
    case 2:
        return V0_11_0_0;
    case 1:
        return V0_10_2_0;
    case 0:
        return V0_10_1_0;
    default:
        return V2_8_0_0;
    }
}

int TopicDetail::encode(PEncoder &pe)
{
    pe.putInt32(m_num_partitions);
    pe.putInt16(m_replication_factor);

    if (pe.putArrayLength(static_cast<int32_t>(m_replica_assignment.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }
    for (auto &partitionAssign : m_replica_assignment)
    {
        pe.putInt32(partitionAssign.first);
        if (pe.putInt32Array(partitionAssign.second) != ErrNoError)
        {
            return ErrEncodeError;
        }
        pe.putEmptyTaggedFieldArray();
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_config_entries.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }
    for (auto &configEntry : m_config_entries)
    {
        if (pe.putString(configEntry.first) != ErrNoError)
        {
            return ErrEncodeError;
        }
        if (pe.putNullableString(configEntry.second) != ErrNoError)
        {
            return ErrEncodeError;
        }
        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int TopicDetail::decode(PDecoder &pd, int16_t version)
{
    if (pd.getInt32(m_num_partitions) != ErrNoError)
    {
        return ErrDecodeError;
    }
    if (pd.getInt16(m_replication_factor) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (n > 0)
    {
        m_replica_assignment.clear();
        for (int32_t i = 0; i < n; ++i)
        {
            int32_t partition;
            if (pd.getInt32(partition) != ErrNoError)
            {
                return ErrDecodeError;
            }
            std::vector<int32_t> replicas;
            if (pd.getInt32Array(replicas) != ErrNoError)
            {
                return ErrDecodeError;
            }
            m_replica_assignment[partition] = std::move(replicas);
            int32_t _;
            if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
            {
                return ErrDecodeError;
            }
        }
    }

    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (n > 0)
    {
        m_config_entries.clear();
        for (int32_t i = 0; i < n; ++i)
        {
            std::string key;
            if (pd.getString(key) != ErrNoError)
            {
                return ErrDecodeError;
            }
            std::string value;
            if (pd.getNullableString(value) != ErrNoError)
            {
                return ErrDecodeError;
            }
            m_config_entries[key] = value;
            int32_t _;
            if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
            {
                return ErrDecodeError;
            }
        }
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

std::shared_ptr<CreateTopicsRequest> NewCreateTopicsRequest(const KafkaVersion &Version_, std::map<std::string, std::shared_ptr<TopicDetail>> topicDetails, int64_t timeoutMs, bool validateOnly)
{
    auto r = std::make_shared<CreateTopicsRequest>();
    r->m_topic_details = topicDetails;
    r->m_timeout = std::chrono::milliseconds(timeoutMs);
    r->m_validate_only = validateOnly;

    if (Version_ >= V2_4_0_0)
    {
        r->m_version = 5;
    }
    else if (Version_ >= V2_0_0_0)
    {
        r->m_version = 3;
    }
    else if (Version_ >= V0_11_0_0)
    {
        r->m_version = 2;
    }
    else if (Version_ >= V0_10_2_0)
    {
        r->m_version = 1;
    }
    else
    {
        r->m_version = 0;
    }

    return r;
}