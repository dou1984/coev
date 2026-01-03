#include "version.h"
#include "create_topics_response.h"
#include <stdexcept>
#include <sstream>

void CreateTopicsResponse::setVersion(int16_t v)
{
    Version = v;
}

int CreateTopicsResponse::encode(PEncoder &pe)
{
    if (Version >= 2)
    {
        pe.putDurationMs(ThrottleTime);
    }

    if (!pe.putArrayLength(static_cast<int32_t>(TopicErrors.size())))
    {
        return ErrEncodeError;
    }

    for (auto &kv : TopicErrors)
    {
        const std::string &topic = kv.first;
        auto &topicError = kv.second;

        if (!pe.putString(topic))
        {
            return ErrEncodeError;
        }
        if (!topicError->encode(pe, Version))
        {
            return ErrEncodeError;
        }

        if (Version >= 5)
        {
            auto it = TopicResults.find(topic);
            if (it == TopicResults.end())
            {

                return ErrEncodeError;
            }
            if (!it->second->encode(pe, Version))
            {
                return ErrEncodeError;
            }
        }
    }

    pe.putEmptyTaggedFieldArray();
    return true;
}

int CreateTopicsResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    if (version >= 2)
    {
        if (pd.getDurationMs(ThrottleTime) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    TopicErrors.clear();
    if (version >= 5)
    {
        TopicResults.clear();
    }

    for (int32_t i = 0; i < n; ++i)
    {
        std::string topic;
        if (pd.getString(topic) != ErrNoError)
        {
            return ErrDecodeError;
        }

        auto topicErr = std::make_shared<TopicError>();
        if (!topicErr->decode(pd, version))
        {
            return ErrDecodeError;
        }
        TopicErrors[topic] = topicErr;

        if (version >= 5)
        {
            auto result = std::make_shared<CreatableTopicResult>();
            if (!result->decode(pd, version))
            {
                return ErrDecodeError;
            }
            TopicResults[topic] = result;
        }
    }

    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t CreateTopicsResponse::key() const
{
    return 19; // apiKeyCreateTopics
}

int16_t CreateTopicsResponse::version() const
{
    return Version;
}

int16_t CreateTopicsResponse::headerVersion() const
{
    return Version >= 5 ? 1 : 0;
}

bool CreateTopicsResponse::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool CreateTopicsResponse::isFlexibleVersion(int16_t version)
{
    return version >= 5;
}

bool CreateTopicsResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 5;
}

KafkaVersion CreateTopicsResponse::requiredVersion() const
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

std::chrono::milliseconds CreateTopicsResponse::throttleTime() const
{
    return ThrottleTime;
}

// === TopicError ===

std::string TopicError::Error() const
{
    std::ostringstream oss;
    // 假设 KError 可以通过整数表示错误类型，此处简化处理
    oss << "Kafka error code " << static_cast<int>(Err);
    if (!ErrMsg.empty())
    {
        oss << " - " << ErrMsg;
    }
    return oss.str();
}

int TopicError::encode(PEncoder &pe, int16_t version)
{
    pe.putInt16(Err);

    if (version >= 1)
    {
        if (!pe.putNullableString(ErrMsg))
        {
            return ErrEncodeError;
        }
    }

    return true;
}

int TopicError::decode(PDecoder &pd, int16_t version)
{
    int16_t errCode;
    if (pd.getInt16(errCode) != ErrNoError)
    {
        return ErrDecodeError;
    }
    Err = static_cast<KError>(errCode);

    if (version >= 1)
    {

        if (pd.getNullableString(ErrMsg) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    return true;
}

// === CreatableTopicConfigs ===

int CreatableTopicConfigs::encode(PEncoder &pe, int16_t /*version*/)
{
    if (!pe.putNullableString(Value))
    {
        return ErrEncodeError;
    }
    pe.putBool(ReadOnly);
    pe.putInt8(static_cast<int8_t>(ConfigSource_));
    pe.putBool(IsSensitive);
    pe.putEmptyTaggedFieldArray();
    return true;
}

int CreatableTopicConfigs::decode(PDecoder &pd, int16_t /*version*/)
{

    if (pd.getNullableString(Value) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (pd.getBool(ReadOnly) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int8_t source;
    if (pd.getInt8(source) != ErrNoError)
    {
        return ErrDecodeError;
    }
    ConfigSource_ = static_cast<ConfigSource>(source);

    if (pd.getBool(IsSensitive) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

// === CreatableTopicResult ===

int CreatableTopicResult::encode(PEncoder &pe, int16_t /*version*/)
{
    pe.putInt32(NumPartitions);
    pe.putInt16(ReplicationFactor);

    if (!pe.putArrayLength(static_cast<int32_t>(Configs.size())))
    {
        return ErrEncodeError;
    }
    for (auto &kv : Configs)
    {
        if (!pe.putString(kv.first))
        {
            return ErrEncodeError;
        }
        if (!kv.second->encode(pe, 0))
        {
            return ErrEncodeError;
        }
    }

    if (TopicConfigErrorCode == KError::ErrNoError)
    {
        pe.putEmptyTaggedFieldArray();
        return true;
    }

    // 写入带标签字段：1 个字段，tag=0，值为 KError
    pe.putUVarint(1); // num tagged fields
    pe.putUVarint(0); // tag id = 0
    pe.putUVarint(2); // length of value = sizeof(int16_t)
    pe.putInt16(static_cast<int16_t>(TopicConfigErrorCode));

    return true;
}

int CreatableTopicResult::decode(PDecoder &pd, int16_t /*version*/)
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

    Configs.clear();
    for (int32_t i = 0; i < n; ++i)
    {
        std::string name;
        if (pd.getString(name) != ErrNoError)
        {
            return ErrDecodeError;
        }
        auto config = std::make_shared<CreatableTopicConfigs>();
        if (!config->decode(pd, 0))
        {
            return ErrDecodeError;
        }
        Configs[name] = config;
    }

    bool hasTag0 = false;

    std::unordered_map<uint64_t, taggedFieldDecoderFunc> handler;
    handler[0] = [&](PDecoder &inner) -> int
    {
        int16_t err;
        if (!inner.getInt16(err))
        {
            return ErrDecodeError;
        }
        TopicConfigErrorCode = static_cast<KError>(err);
        hasTag0 = true;
        return ErrDecodeError;
    };

    if (pd.getTaggedFieldArray(handler) != 0)
    {
        return ErrDecodeError;
    }

    if (!hasTag0)
    {
        TopicConfigErrorCode = KError::ErrNoError;
    }

    return ErrNoError;
}