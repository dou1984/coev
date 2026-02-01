#include "version.h"
#include "uuid.h"
#include <string>
#include <vector>
#include <cstdint>
#include "metadata_request.h"
#include <stdexcept>

static std::string NullUUID = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static std::string base64UrlEncodeWithoutPadding(const uint8_t *data, size_t len)
{
    static const char kBase64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string result;
    size_t i = 0;
    uint32_t triple;

    while (i < len)
    {
        triple = (static_cast<uint32_t>(data[i]) << 16);
        if (i + 1 < len)
            triple |= (static_cast<uint32_t>(data[i + 1]) << 8);
        if (i + 2 < len)
            triple |= static_cast<uint32_t>(data[i + 2]);

        result.push_back(kBase64Chars[(triple >> 18) & 0x3F]);
        result.push_back(kBase64Chars[(triple >> 12) & 0x3F]);
        if (i + 1 < len)
        {
            result.push_back(kBase64Chars[(triple >> 6) & 0x3F]);
        }
        if (i + 2 < len)
        {
            result.push_back(kBase64Chars[triple & 0x3F]);
        }
        i += 3;
    }
    return result;
}

std::string Uuid::String() const
{
    return base64UrlEncodeWithoutPadding(data.data(), data.size());
}

void MetadataRequest::set_version(int16_t v)
{
    m_version = v;
}

int MetadataRequest::encode(packet_encoder &pe) const
{
    if (m_version < 0 || m_version > 10)
    {
        return -1;
    }
    if (m_version == 0 || !m_topics.empty())
    {
        if (int err = pe.putArrayLength(static_cast<int32_t>(m_topics.size())); err != 0)
        {
            return err;
        }
        if (m_version <= 9)
        {
            for (auto &topicName : m_topics)
            {
                if (int err = pe.putString(topicName); err != 0)
                {
                    return err;
                }
                pe.putEmptyTaggedFieldArray();
            }
        }
        else
        {
            for (auto &topicName : m_topics)
            {
                if (int err = pe.putRawBytes(NullUUID); err != 0)
                {
                    return err;
                }

                if (int err = pe.putNullableString(topicName); err != 0)
                {
                    return err;
                }
                pe.putEmptyTaggedFieldArray();
            }
        }
    }
    else
    {
        if (int err = pe.putArrayLength(-1); err != 0)
        {
            return err;
        }
    }

    if (m_version > 3)
    {
        pe.putBool(m_allow_auto_topic_creation);
    }
    if (m_version > 7)
    {
        pe.putBool(m_include_cluster_authorized_operations);
        pe.putBool(m_include_topic_authorized_operations);
    }
    pe.putEmptyTaggedFieldArray();
    return 0;
}

int MetadataRequest::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;
    int32_t size;
    if (int err = pd.getArrayLength(size); err != 0)
    {
        return err;
    }
    if (size > 0)
    {
        m_topics.resize(size);
    }
    if (version <= 9)
    {
        for (size_t i = 0; i < m_topics.size(); ++i)
        {
            std::string topic;
            if (int err = pd.getString(topic); err != 0)
            {
                return err;
            }
            m_topics[i] = topic;
            int32_t _;
            if (int err = pd.getEmptyTaggedFieldArray(_); err != 0)
            {
                return err;
            }
        }
    }
    else
    {
        for (size_t i = 0; i < m_topics.size(); ++i)
        {
            std::string t;
            if (int err = pd.getRawBytes(16, t); err != 0)
            {
                return err;
            }
            std::string topic;
            if (int err = pd.getNullableString(topic); err != 0)
            {
                return err;
            }
            m_topics[i] = topic;

            int32_t _;
            if (int err = pd.getEmptyTaggedFieldArray(_); err != 0)
            {
                return err;
            }
        }
    }

    if (m_version >= 4)
    {
        if (int err = pd.getBool(m_allow_auto_topic_creation); err != 0)
        {
            return err;
        }
    }

    if (m_version > 7)
    {
        bool includeClusterAuthz;
        if (int err = pd.getBool(includeClusterAuthz); err != 0)
        {
            return err;
        }
        m_include_cluster_authorized_operations = includeClusterAuthz;
        bool includeTopicAuthz;
        if (int err = pd.getBool(includeTopicAuthz); err != 0)
        {
            return err;
        }
        m_include_topic_authorized_operations = includeTopicAuthz;
    }

    int32_t err;
    if (int e = pd.getEmptyTaggedFieldArray(err); e != 0)
    {
        return e;
    }
    return 0;
}

int16_t MetadataRequest::key() const
{
    return apiKeyMetadata;
}

int16_t MetadataRequest::version() const
{
    return m_version;
}

int16_t MetadataRequest::header_version() const
{
    if (m_version >= 9)
    {
        return 2;
    }
    return 1;
}

bool MetadataRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 10;
}

bool MetadataRequest::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool MetadataRequest::is_flexible_version(int16_t version) const
{
    return version >= 9;
}

KafkaVersion MetadataRequest::required_version() const
{
    switch (m_version)
    {
    case 10:
        return V2_8_0_0;
    case 9:
        return V2_4_0_0;
    case 8:
        return V2_3_0_0;
    case 7:
        return V2_1_0_0;
    case 6:
        return V2_0_0_0;
    case 5:
        return V1_0_0_0;
    case 3:
    case 4:
        return V0_11_0_0;
    case 2:
        return V0_10_1_0;
    case 1:
        return V0_10_0_0;
    case 0:
        return V0_8_2_0;
    default:
        return V2_8_0_0;
    }
}

MetadataRequest::MetadataRequest(KafkaVersion version, const std::vector<std::string> &topics)
{
    m_topics = topics;
    if (version.IsAtLeast(V2_8_0_0))
    {
        m_version = 10;
    }
    else if (version.IsAtLeast(V2_4_0_0))
    {
        m_version = 9;
    }
    else if (version.IsAtLeast(V2_4_0_0))
    {
        m_version = 8;
    }
    else if (version.IsAtLeast(V2_1_0_0))
    {
        m_version = 7;
    }
    else if (version.IsAtLeast(V2_0_0_0))
    {
        m_version = 6;
    }
    else if (version.IsAtLeast(V1_0_0_0))
    {
        m_version = 5;
    }
    else if (version.IsAtLeast(V0_11_0_0))
    {
        m_version = 4;
    }
    else if (version.IsAtLeast(V0_10_1_0))
    {
        m_version = 2;
    }
    else if (version.IsAtLeast(V0_10_0_0))
    {
        m_version = 1;
    }
}
