#include "partition_error.h"

int PartitionError::encode(PEncoder &pe)
{
    pe.putInt32(Partition);
    pe.putKError(Err);
    return 0;
}

int PartitionError::decode(PDecoder &pd, int16_t version)
{
    int err;
    if ((err = pd.getInt32(Partition)) != 0)
    {
        return err;
    }

    if ((err = pd.getKError(Err)) != 0)
    {
        return err;
    }

    return 0;
}
