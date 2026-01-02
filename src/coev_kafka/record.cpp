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
    pe.push(std::dynamic_pointer_cast<pushEncoder>(length));
    pe.putInt8(Attributes);
    pe.putVariant(TimestampDelta.count());
    pe.putVariant(OffsetDelta);
    if (pe.putVariantBytes(Key) != 0)
    {
        return -1;
    }
    if (pe.putVariantBytes(Value) != 0)
    {
        return -1;
    }
    pe.putVariant(static_cast<int64_t>(Headers.size()));

    for (auto &h : Headers)
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
    if (pd.push(std::dynamic_pointer_cast<pushDecoder>(length)) != 0)
    {
        return -1;
    }

    if (pd.getInt8(Attributes) != 0)
    {
        return -1;
    }

    int64_t timestamp;
    if (pd.getVariant(timestamp) != 0)
    {
        return -1;
    }
    TimestampDelta = std::chrono::milliseconds(timestamp);

    if (pd.getVariant(OffsetDelta) != 0)
    {
        return -1;
    }

    if (pd.getVariantBytes(Key) != 0)
    {
        return -1;
    }

    if (pd.getVariantBytes(Value) != 0)
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
        Headers.resize(numHeaders);
        for (int64_t i = 0; i < numHeaders; ++i)
        {
            Headers[i] = std::make_shared<RecordHeader>();
            if (Headers[i]->decode(pd) != 0)
            {
                return -1;
            }
        }
    }

    return pd.pop();
}