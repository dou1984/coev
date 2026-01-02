#include "version.h"
#include "describe_configs_request.h"

void DescribeConfigsRequest::setVersion(int16_t v)
{
    Version = v;
}

int DescribeConfigsRequest::encode(PEncoder &pe)
{
    if (!pe.putArrayLength(static_cast<int32_t>(Resources.size())))
    {
        return ErrEncodeError;
    }

    for (auto &c : Resources)
    {
        pe.putInt8(static_cast<int8_t>(c->Type));
        if (!pe.putString(c->Name))
        {
            return ErrEncodeError;
        }

        if (c->ConfigNames.empty())
        {
            pe.putInt32(-1);
            continue;
        }

        if (!pe.putStringArray(c->ConfigNames))
        {
            return ErrEncodeError;
        }
    }

    if (Version >= 1)
    {
        pe.putBool(IncludeSynonyms);
    }

    return true;
}

int DescribeConfigsRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    int32_t n;
    if (!pd.getArrayLength(n))
    {
        return ErrDecodeError;
    }

    Resources.clear();
    Resources.resize(n);

    for (int32_t i = 0; i < n; ++i)
    {
        Resources[i] = std::make_shared<ConfigResource>();

        int8_t t;
        if (!pd.getInt8(t))
        {
            return ErrDecodeError;
        }
        Resources[i]->Type = static_cast<ConfigResourceType>(t);

        std::string name;
        if (!pd.getString(name))
        {
            return ErrDecodeError;
        }
        Resources[i]->Name = name;

        int32_t confLength;
        if (!pd.getArrayLength(confLength))
        {
            return ErrDecodeError;
        }

        if (confLength == -1)
        {
            continue;
        }

        Resources[i]->ConfigNames.resize(confLength);
        for (int32_t j = 0; j < confLength; ++j)
        {
            std::string s;
            if (!pd.getString(s))
            {
                return ErrDecodeError;
            }
            Resources[i]->ConfigNames[j] = s;
        }
    }

    if (Version >= 1)
    {
        bool b;
        if (!pd.getBool(b))
        {
            return ErrDecodeError;
        }
        IncludeSynonyms = b;
    }

    return ErrNoError;
}

int16_t DescribeConfigsRequest::key() const
{
    return apiKeyDescribeConfigs;
}

int16_t DescribeConfigsRequest::version() const
{
    return Version;
}

int16_t DescribeConfigsRequest::headerVersion() const
{
    return 1;
}

bool DescribeConfigsRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 2;
}

KafkaVersion DescribeConfigsRequest::requiredVersion() const
{
    switch (Version)
    {
    case 2:
        return V2_0_0_0;
    case 1:
        return V1_1_0_0;
    case 0:
        return V0_11_0_0;
    default:
        return V2_0_0_0;
    }
}