#include "partition_offset_metadata.h"

PartitionOffsetMetadata::PartitionOffsetMetadata(int32_t partition, int64_t offset, int32_t leader_epoch,const std::string& metadata) : m_partition(partition), m_offset(offset), m_leader_epoch(leader_epoch), m_metadata(metadata)
{
}
int PartitionOffsetMetadata::decode(packet_decoder &pd, int16_t version)
{
    pd.getInt32(m_partition);
    pd.getInt64(m_offset);
    pd.getString(m_metadata);
    pd.getInt32(m_leader_epoch);
    return 0;
}
int PartitionOffsetMetadata::encode(packet_encoder &pe, int16_t version) const
{
    pe.putInt32(m_partition);
    pe.putInt64(m_offset);
    pe.putString(m_metadata);
    pe.putInt32(m_leader_epoch);
    return 0;
}