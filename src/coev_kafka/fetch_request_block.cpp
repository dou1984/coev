#include "version.h"
#include "fetch_request_block.h"

FetchRequestBlock::FetchRequestBlock()
    : m_version(0), m_current_leader_epoch(0), m_fetch_offset(0), m_log_start_offset(0), m_max_bytes(0) {}

int FetchRequestBlock::encode(packet_encoder &pe, int16_t version) const
{
    if (version >= 9)
    {
        pe.putInt32(m_current_leader_epoch);
    }
    pe.putInt64(m_fetch_offset);
    if (version >= 1)
    {
        pe.putInt64(m_log_start_offset);
    }
    pe.putInt32(m_max_bytes);
    return 0;
}

int FetchRequestBlock::decode(packet_decoder &pd, int16_t version)
{
    int err = 0;
    if (version >= 9)
    {
        if ((err = pd.getInt32(m_current_leader_epoch)) != 0)
        {
            return err;
        }
    }
    if ((err = pd.getInt64(m_fetch_offset)) != 0)
    {
        return err;
    }
    if (version >= 1)
    {
        if ((err = pd.getInt64(m_log_start_offset)) != 0)
        {
            return err;
        }
    }
    if ((err = pd.getInt32(m_max_bytes)) != 0)
    {
        return err;
    }
    return 0;
}