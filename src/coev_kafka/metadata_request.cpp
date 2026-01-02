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

void MetadataRequest::setVersion(int16_t v)
{
    Version = v;
}

int MetadataRequest::encode(PEncoder &pe)
{
    if (Version < 0 || Version > 10)
    {
        return -1;
    }
    if (Version == 0 || !Topics.empty())
    {
        if (int err = pe.putArrayLength(static_cast<int32_t>(Topics.size())); err != 0)
        {
            return err;
        }
        if (Version <= 9)
        {
            for (auto &topicName : Topics)
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
            for (auto &topicName : Topics)
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

    if (Version > 3)
    {
        pe.putBool(AllowAutoTopicCreation);
    }
    if (Version > 7)
    {
        pe.putBool(IncludeClusterAuthorizedOperations);
        pe.putBool(IncludeTopicAuthorizedOperations);
    }
    pe.putEmptyTaggedFieldArray();
    return 0;
}

int MetadataRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    int32_t size;
    if (int err = pd.getArrayLength(size); err != 0)
    {
        return err;
    }
    if (size > 0)
    {
        Topics.resize(size);
    }
    if (version <= 9)
    {
        for (size_t i = 0; i < Topics.size(); ++i)
        {
            std::string topic;
            if (int err = pd.getString(topic); err != 0)
            {
                return err;
            }
            Topics[i] = topic;
            int32_t _;
            if (int err = pd.getEmptyTaggedFieldArray(_); err != 0)
            {
                return err;
            }
        }
    }
    else
    {
        for (size_t i = 0; i < Topics.size(); ++i)
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
            Topics[i] = topic;

            int32_t _;
            if (int err = pd.getEmptyTaggedFieldArray(_); err != 0)
            {
                return err;
            }
        }
    }

    if (Version >= 4)
    {
        if (int err = pd.getBool(AllowAutoTopicCreation); err != 0)
        {
            return err;
        }
    }

    if (Version > 7)
    {
        bool includeClusterAuthz;
        if (int err = pd.getBool(includeClusterAuthz); err != 0)
        {
            return err;
        }
        IncludeClusterAuthorizedOperations = includeClusterAuthz;
        bool includeTopicAuthz;
        if (int err = pd.getBool(includeTopicAuthz); err != 0)
        {
            return err;
        }
        IncludeTopicAuthorizedOperations = includeTopicAuthz;
    }

    int err;
    pd.getEmptyTaggedFieldArray(err);
    return err;
}

int16_t MetadataRequest::key() const
{
    return apiKeyMetadata;
}

int16_t MetadataRequest::version() const
{
    return Version;
}

int16_t MetadataRequest::headerVersion() const
{
    if (Version >= 9)
    {
        return 2;
    }
    return 1;
}

bool MetadataRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 10;
}

bool MetadataRequest::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool MetadataRequest::isFlexibleVersion(int16_t version) const
{
    return version >= 9;
}

KafkaVersion MetadataRequest::requiredVersion() const
{
    switch (Version)
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

std::shared_ptr<MetadataRequest> NewMetadataRequest(KafkaVersion version, const std::vector<std::string> &topics)
{
    auto m = std::make_shared<MetadataRequest>();
    m->Topics = topics;
    if (version.IsAtLeast(V2_8_0_0))
    {
        m->Version = 10;
    }
    else if (version.IsAtLeast(V2_4_0_0))
    {
        m->Version = 9;
    }
    else if (version.IsAtLeast(V2_4_0_0))
    {
        m->Version = 8;
    }
    else if (version.IsAtLeast(V2_1_0_0))
    {
        m->Version = 7;
    }
    else if (version.IsAtLeast(V2_0_0_0))
    {
        m->Version = 6;
    }
    else if (version.IsAtLeast(V1_0_0_0))
    {
        m->Version = 5;
    }
    else if (version.IsAtLeast(V0_11_0_0))
    {
        m->Version = 4;
    }
    else if (version.IsAtLeast(V0_10_1_0))
    {
        m->Version = 2;
    }
    else if (version.IsAtLeast(V0_10_0_0))
    {
        m->Version = 1;
    }
    return m;
}