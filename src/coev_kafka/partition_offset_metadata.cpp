#include "partition_offset_metadata.h"

int PartitionOffsetMetadata::decode(PDecoder &pd, int16_t version)
{
    pd.getInt32(m_partition);
    pd.getInt64(m_offset);
    pd.getString(m_metadata);
    pd.getInt32(m_leader_epoch);
    return 0;
}
int PartitionOffsetMetadata::encode(PEncoder &pe, int16_t version)
{
    pe.putInt32(m_partition);
    pe.putInt64(m_offset);
    pe.putString(m_metadata);
    pe.putInt32(m_leader_epoch);
    return 0;
}