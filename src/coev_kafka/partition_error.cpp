/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include "partition_error.h"

namespace coev::kafka
{

    int PartitionError::encode(packet_encoder &pe) const
    {
        pe.putInt32(m_partition);
        pe.putKError(m_err);
        return 0;
    }

    int PartitionError::decode(packet_decoder &pd, int16_t version)
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

} // namespace coev::kafka
