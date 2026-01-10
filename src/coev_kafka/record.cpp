#include "record.h"

int RecordHeader::encode(PEncoder &pe)
{
    if (pe.putVariantBytes(Key) != 0)
    {
        return -1;
    }
    if (pe.putVariantBytes(Value) != 0)
    {
        return -1;
    }
    return 0;
}

int RecordHeader::decode(PDecoder &pd)
{
    if (pd.getVariantBytes(Key) != 0)
    {
        return -1;
    }
    if (pd.getVariantBytes(Value) != 0)
    {
        return -1;
    }
    return 0;
}

int Record::encode(PEncoder &pe)
{
    pe.push(std::dynamic_pointer_cast<pushEncoder>(m_length));
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
        if (h->encode(pe) != 0)
        {
            return -1;
        }
    }

    return pe.pop();
}

int Record::decode(PDecoder &pd)
{
    if (pd.push(std::dynamic_pointer_cast<pushDecoder>(m_length)) != 0)
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
            m_headers[i] = std::make_shared<RecordHeader>();
            if (m_headers[i]->decode(pd) != 0)
            {
                return -1;
            }
        }
    }

    return pd.pop();
}