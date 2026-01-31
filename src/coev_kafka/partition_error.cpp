#include "partition_error.h"

int PartitionError::encode(packetEncoder &pe) const
{
    pe.putInt32(m_partition);
    pe.putKError(m_err);
    return 0;
}

int PartitionError::decode(packetDecoder &pd, int16_t version)
{
    int err;
    if ((err = pd.getInt32(m_partition)) != 0)
    {
        return err;
    }

    if ((err = pd.getKError(m_err)) != 0)
    {
        return err;
    }

    return 0;
}
