#include "version.h"
#include "create_topics_response.h"
#include <stdexcept>
#include <sstream>

void CreateTopicsResponse::set_version(int16_t v)
{
    m_version = v;
}

int CreateTopicsResponse::encode(PEncoder &pe)
{
    if (m_version >= 2)
    {
        pe.putDurationMs(m_throttle_time);
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_topic_errors.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &kv : m_topic_errors)
    {
        const std::string &topic = kv.first;
        auto &topicError = kv.second;

        if (pe.putString(topic) != ErrNoError)
        {
            return ErrEncodeError;
        }
        if (topicError->encode(pe, m_version) != ErrNoError)
        {
            return ErrEncodeError;
        }

        if (m_version >= 5)
        {
            auto it = m_topic_results.find(topic);
            if (it == m_topic_results.end())
            {

                return ErrEncodeError;
            }
            if (it->second->encode(pe, m_version) != ErrNoError)
            {
                return ErrEncodeError;
            }
        }
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int CreateTopicsResponse::decode(PDecoder &pd, int16_t version)
{
    m_version = version;

    if (version >= 2)
    {
        if (pd.getDurationMs(m_throttle_time) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_topic_errors.clear();
    if (version >= 5)
    {
        m_topic_results.clear();
    }

    for (int32_t i = 0; i < n; ++i)
    {
        std::string topic;
        if (pd.getString(topic) != ErrNoError)
        {
            return ErrDecodeError;
        }

        auto topicErr = std::make_shared<TopicError>();
        if (topicErr->decode(pd, version) != ErrNoError)
        {
            return ErrDecodeError;
        }
        m_topic_errors[topic] = topicErr;

        if (version >= 5)
        {
            auto result = std::make_shared<CreatableTopicResult>();
            if (result->decode(pd, version) != ErrNoError)
            {
                return ErrDecodeError;
            }
            m_topic_results[topic] = result;
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
    return m_version;
}

int16_t CreateTopicsResponse::header_version() const
{
    return m_version >= 5 ? 1 : 0;
}

bool CreateTopicsResponse::isFlexible() const
{
    return isFlexibleVersion(m_version);
}

bool CreateTopicsResponse::isFlexibleVersion(int16_t version)
{
    return version >= 5;
}

bool CreateTopicsResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 5;
}

KafkaVersion CreateTopicsResponse::required_version() const
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

std::chrono::milliseconds CreateTopicsResponse::throttle_time() const
{
    return m_throttle_time;
}

// === TopicError ===

std::string TopicError::Error() const
{
    std::ostringstream oss;
    // 假设 KError 可以通过整数表示错误类型，此处简化处理
    oss << "Kafka error code " << static_cast<int>(m_err);
    if (!m_err_msg.empty())
    {
        oss << " - " << m_err_msg;
    }
    return oss.str();
}

int TopicError::encode(PEncoder &pe, int16_t version)
{
    pe.putInt16(m_err);

    if (version >= 1)
    {
        if (pe.putNullableString(m_err_msg) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    return ErrNoError;
}

int TopicError::decode(PDecoder &pd, int16_t version)
{
    int16_t errCode;
    if (pd.getInt16(errCode) != ErrNoError)
    {
        return ErrDecodeError;
    }
    m_err = static_cast<KError>(errCode);

    if (version >= 1)
    {

        if (pd.getNullableString(m_err_msg) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    return ErrNoError;
}

// === CreatableTopicConfigs ===

int CreatableTopicConfigs::encode(PEncoder &pe, int16_t /*version*/)
{
    if (pe.putNullableString(m_value) != ErrNoError)
    {
        return ErrEncodeError;
    }
    pe.putBool(m_read_only);
    pe.putInt8(static_cast<int8_t>(m_config_source));
    pe.putBool(m_is_sensitive);
    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int CreatableTopicConfigs::decode(PDecoder &pd, int16_t /*version*/)
{

    if (pd.getNullableString(m_value) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (pd.getBool(m_read_only) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int8_t source;
    if (pd.getInt8(source) != ErrNoError)
    {
        return ErrDecodeError;
    }
    m_config_source = static_cast<ConfigSource>(source);

    if (pd.getBool(m_is_sensitive) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

// === CreatableTopicResult ===

int CreatableTopicResult::encode(PEncoder &pe, int16_t /*version*/)
{
    pe.putInt32(m_num_partitions);
    pe.putInt16(m_replication_factor);

    if (pe.putArrayLength(static_cast<int32_t>(m_configs.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }
    for (auto &kv : m_configs)
    {
        if (pe.putString(kv.first) != ErrNoError)
        {
            return ErrEncodeError;
        }
        if (kv.second->encode(pe, 0) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    if (m_topic_config_error_code == KError::ErrNoError)
    {
        pe.putEmptyTaggedFieldArray();
        return ErrNoError;
    }

    // 写入带标签字段：1 个字段，tag=0，值为 KError
    pe.putUVarint(1); // num tagged fields
    pe.putUVarint(0); // tag id = 0
    pe.putUVarint(2); // length of value = sizeof(int16_t)
    pe.putInt16(static_cast<int16_t>(m_topic_config_error_code));

    return ErrNoError;
}

int CreatableTopicResult::decode(PDecoder &pd, int16_t /*version*/)
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

    m_configs.clear();
    for (int32_t i = 0; i < n; ++i)
    {
        std::string name;
        if (pd.getString(name) != ErrNoError)
        {
            return ErrDecodeError;
        }
        auto config = std::make_shared<CreatableTopicConfigs>();
        if (config->decode(pd, 0) != ErrNoError)
        {
            return ErrDecodeError;
        }
        m_configs[name] = config;
    }

    bool hasTag0 = false;

    std::unordered_map<uint64_t, taggedFieldDecoderFunc> handler;
    handler[0] = [&](PDecoder &inner) -> int
    {
        int16_t err;
        if (inner.getInt16(err) != ErrNoError)
        {
            return ErrDecodeError;
        }
        m_topic_config_error_code = static_cast<KError>(err);
        hasTag0 = true;
        return ErrNoError;
    };

    if (pd.getTaggedFieldArray(handler) != 0)
    {
        return ErrDecodeError;
    }

    if (!hasTag0)
    {
        m_topic_config_error_code = KError::ErrNoError;
    }

    return ErrNoError;
}