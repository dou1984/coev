#include "api_versions.h"
#include "create_topics_request.h"

void CreateTopicsRequest::setVersion(int16_t v)
{
    Version = v;
}

int CreateTopicsRequest::encode(PEncoder &pe)
{
    if (!pe.putArrayLength(static_cast<int32_t>(TopicDetails.size())))
    {
        return ErrEncodeError;
    }

    for (auto &kv : TopicDetails)
    {
        if (!pe.putString(kv.first))
        {
            return ErrEncodeError;
        }
        if (!kv.second->encode(pe))
        {
            return ErrEncodeError;
        }
    }

    pe.putDurationMs(Timeout);

    if (Version >= 1)
    {
        pe.putBool(ValidateOnly);
    }

    pe.putEmptyTaggedFieldArray();
    return true;
}

int CreateTopicsRequest::decode(PDecoder &pd, int16_t version)
{
    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    TopicDetails.clear();
    for (int32_t i = 0; i < n; ++i)
    {
        std::string topic;
        if (pd.getString(topic) != ErrNoError)
        {
            return ErrDecodeError;
        }
        auto detail = std::make_shared<TopicDetail>();
        if (!detail->decode(pd, version))
        {
            return ErrDecodeError;
        }
        TopicDetails[topic] = detail;
    }

    if (pd.getDurationMs(Timeout) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (version >= 1)
    {
        if (pd.getBool(ValidateOnly) != ErrNoError)
        {
            return ErrDecodeError;
        }
        Version = version;
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
    return Version;
}

int16_t CreateTopicsRequest::headerVersion() const
{
    return Version >= 5 ? 2 : 1;
}

bool CreateTopicsRequest::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool CreateTopicsRequest::isFlexibleVersion(int16_t version)
{
    return version >= 5;
}

bool CreateTopicsRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 5;
}

KafkaVersion CreateTopicsRequest::requiredVersion() const
{
    switch (Version)
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
    pe.putInt32(NumPartitions);
    pe.putInt16(ReplicationFactor);

    if (!pe.putArrayLength(static_cast<int32_t>(ReplicaAssignment.size())))
    {
        return ErrEncodeError;
    }
    for (auto &partitionAssign : ReplicaAssignment)
    {
        pe.putInt32(partitionAssign.first);
        if (!pe.putInt32Array(partitionAssign.second))
        {
            return ErrEncodeError;
        }
        pe.putEmptyTaggedFieldArray();
    }

    if (!pe.putArrayLength(static_cast<int32_t>(ConfigEntries.size())))
    {
        return ErrEncodeError;
    }
    for (auto &configEntry : ConfigEntries)
    {
        if (!pe.putString(configEntry.first))
        {
            return ErrEncodeError;
        }
        if (!pe.putNullableString(configEntry.second))
        {
            return ErrEncodeError;
        }
        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return true;
}

int TopicDetail::decode(PDecoder &pd, int16_t version)
{
    if (pd.getInt32(NumPartitions) != ErrNoError)
    {
        return ErrDecodeError;
    }
    if (pd.getInt16(ReplicationFactor) != ErrNoError)
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
        ReplicaAssignment.clear();
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
            ReplicaAssignment[partition] = std::move(replicas);
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
        ConfigEntries.clear();
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
            ConfigEntries[key] = value;
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
    r->TopicDetails = topicDetails;
    r->Timeout = std::chrono::milliseconds(timeoutMs);
    r->ValidateOnly = validateOnly;

    if (Version_ >= V2_4_0_0)
    {
        r->Version = 5;
    }
    else if (Version_ >= V2_0_0_0)
    {
        r->Version = 3;
    }
    else if (Version_ >= V0_11_0_0)
    {
        r->Version = 2;
    }
    else if (Version_ >= V0_10_2_0)
    {
        r->Version = 1;
    }
    else
    {
        r->Version = 0;
    }

    return r;
}