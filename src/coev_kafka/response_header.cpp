#include "response_header.h"
#include <string>
#include "real_decoder.h"

template <class T>
int decode(T &pd, int32_t &length, int32_t &correlationID)
{
    int err = pd.getInt32(length);
    if (err != 0)
    {
        return err;
    }

    if (length <= 4 || length > MaxResponseSize)
    {
        return -1;
    }

    err = pd.getInt32(correlationID);
    if (err != 0)
    {
        return err;
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}
int ResponseHeader::decode(packet_decoder &pd, int16_t version)
{
    if (version >= 1)
    {
        auto decoder = static_cast<real_decoder *>(&pd);
        if (decoder != nullptr)
        {
            decoder->_push_flexible();
            defer(decoder->_pop_flexible());
            return ::decode(*decoder, m_length, m_correlation_id);
        }
        else
        {
            LOG_CORE("[ResponseHeader] decoder is nullptr");
            return -1;
        }
    }
    return ::decode(pd, m_length, m_correlation_id);
}