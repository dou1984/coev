#include "fetch_request_block.h"

FetchRequestBlock::FetchRequestBlock()
    : Version(0), currentLeaderEpoch(0), fetchOffset(0), logStartOffset(0), maxBytes(0) {}

int FetchRequestBlock::encode(PEncoder &pe, int16_t version)
{
    Version = version;
    if (Version >= 9)
    {
        pe.putInt32(currentLeaderEpoch);
    }
    pe.putInt64(fetchOffset);
    if (Version >= 5)
    {
        pe.putInt64(logStartOffset);
    }
    pe.putInt32(maxBytes);
    return 0;
}

int FetchRequestBlock::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    int err = 0;
    if (Version >= 9)
    {
        if ((err = pd.getInt32(currentLeaderEpoch)) != 0)
        {
            return err;
        }
    }
    if ((err = pd.getInt64(fetchOffset)) != 0)
    {
        return err;
    }
    if (Version >= 5)
    {
        if ((err = pd.getInt64(logStartOffset)) != 0)
        {
            return err;
        }
    }
    if ((err = pd.getInt32(maxBytes)) != 0)
    {
        return err;
    }
    return 0;
}