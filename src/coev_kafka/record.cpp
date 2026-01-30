#include "record.h"

RecordHeader::RecordHeader(const std::string &k, const std::string &v) : m_key(k), m_value(v)
{
}
int RecordHeader::encode(packetEncoder &pe)
{
    if (pe.putVariantBytes(m_key) != 0)
    {
        return -1;
    }
    if (pe.putVariantBytes(m_value) != 0)
    {
        return -1;
    }
    return 0;
}

int RecordHeader::decode(packetDecoder &pd)
{
    if (pd.getVariantBytes(m_key) != 0)
    {
        return -1;
    }
    if (pd.getVariantBytes(m_value) != 0)
    {
        return -1;
    }
    return 0;
}

int Record::encode(packetEncoder &pe)
{
    pe.push(m_length);
    pe.putInt8(m_attributes);
    pe.putVariant(m_timestamp_delta.count());
    pe.putVariant(m_offset_delta);
    if (pe.putVariantBytes(m_key) != 0)
    {
        return -1;
    }
    if (pe.putVariantBytes(m_value) != 0)
    {
        return -1;
    }
    pe.putVariant(static_cast<int64_t>(m_headers.size()));

    for (auto &h : m_headers)
    {
        if (h.encode(pe) != 0)
        {
            return -1;
        }
    }

    return pe.pop();
}

int Record::decode(packetDecoder &pd)
{
    if (pd.push(m_length) != 0)
    {
        return -1;
    }

    if (pd.getInt8(m_attributes) != 0)
    {
        return -1;
    }

    int64_t timestamp;
    if (pd.getVariant(timestamp) != 0)
    {
        return -1;
    }
    m_timestamp_delta = std::chrono::milliseconds(timestamp);

    if (pd.getVariant(m_offset_delta) != 0)
    {
        return -1;
    }

    if (pd.getVariantBytes(m_key) != 0)
    {
        return -1;
    }

    if (pd.getVariantBytes(m_value) != 0)
    {
        return -1;
    }

    int64_t numHeaders;
    if (pd.getVariant(numHeaders) != 0)
    {
        return -1;
    }

    if (numHeaders >= 0)
    {
        m_headers.resize(numHeaders);
        for (int64_t i = 0; i < numHeaders; ++i)
        {
            if (m_headers[i].decode(pd) != 0)
            {
                return -1;
            }
        }
    }

    return pd.pop();
}