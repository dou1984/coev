#include "version.h"
#include "describe_configs_request.h"

void DescribeConfigsRequest::set_version(int16_t v)
{
    m_version = v;
}

int DescribeConfigsRequest::encode(packetEncoder &pe)
{
    if (pe.putArrayLength(static_cast<int32_t>(m_resources.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &c : m_resources)
    {
        pe.putInt8(static_cast<int8_t>(c->m_type));
        if (pe.putString(c->m_name) != ErrNoError)
        {
            return ErrEncodeError;
        }

        if (c->m_config_names.empty())
        {
            pe.putInt32(-1);
            continue;
        }

        if (pe.putStringArray(c->m_config_names) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    if (m_version >= 1)
    {
        pe.putBool(m_include_synonyms);
    }

    return ErrNoError;
}

int DescribeConfigsRequest::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_resources.clear();
    m_resources.resize(n);

    for (int32_t i = 0; i < n; ++i)
    {
        m_resources[i] = std::make_shared<ConfigResource>();

        int8_t t;
        if (pd.getInt8(t) != ErrNoError)
        {
            return ErrDecodeError;
        }
        m_resources[i]->m_type = static_cast<ConfigResourceType>(t);

        std::string name;
        if (pd.getString(name) != ErrNoError)
        {
            return ErrDecodeError;
        }
        m_resources[i]->m_name = name;

        int32_t confLength;
        if (pd.getArrayLength(confLength) != ErrNoError)
        {
            return ErrDecodeError;
        }

        if (confLength == -1)
        {
            continue;
        }

        m_resources[i]->m_config_names.resize(confLength);
        for (int32_t j = 0; j < confLength; ++j)
        {
            std::string s;
            if (pd.getString(s) != ErrNoError)
            {
                return ErrDecodeError;
            }
            m_resources[i]->m_config_names[j] = s;
        }
    }

    if (m_version >= 1)
    {
        bool b;
        if (pd.getBool(b) != ErrNoError)
        {
            return ErrDecodeError;
        }
        m_include_synonyms = b;
    }

    return ErrNoError;
}

int16_t DescribeConfigsRequest::key() const
{
    return apiKeyDescribeConfigs;
}

int16_t DescribeConfigsRequest::version() const
{
    return m_version;
}

int16_t DescribeConfigsRequest::header_version() const
{
    return 1;
}

bool DescribeConfigsRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 2;
}

KafkaVersion DescribeConfigsRequest::required_version() const
{
    switch (m_version)
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