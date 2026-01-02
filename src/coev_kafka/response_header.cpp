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
int responseHeader::decode(PDecoder &pd, int16_t version)
{

    if (version >= 1)
    {
        if (auto decoder = dynamic_cast<realFlexibleDecoder *>(&pd))
        {
            return ::decode(*decoder, length, correlationID);
        }
        else
        {
            return -1;
        }
    }
    return ::decode(pd, length, correlationID);
}