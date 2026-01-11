#include "version.h"
#include "add_offsets_to_txn_request.h"

void AddOffsetsToTxnRequest::set_version(int16_t v)
{
    m_version = v;
}

int AddOffsetsToTxnRequest::encode(PEncoder &pe)
{
    if (pe.putString(m_transactional_id) != 0)
    {
        return -1;
    }

    pe.putInt64(m_producer_id);
    pe.putInt16(m_producer_epoch);

    if (pe.putString(m_group_id) != 0)
    {
        return -1;
    }

    return 0;
}

int AddOffsetsToTxnRequest::decode(PDecoder &pd, int16_t version)
{
    int err;
    if ((err = pd.getString(m_transactional_id)) != 0)
    {
        return err;
    }
    if ((err = pd.getInt64(m_producer_id)) != 0)
    {
        return err;
    }
    if ((err = pd.getInt16(m_producer_epoch)) != 0)
    {
        return err;
    }
    if ((err = pd.getString(m_group_id)) != 0)
    {
        return err;
    }
    return 0;
}

int16_t AddOffsetsToTxnRequest::key() const
{
    return apiKeyAddOffsetsToTxn;
}

int16_t AddOffsetsToTxnRequest::version() const
{
    return m_version;
}

int16_t AddOffsetsToTxnRequest::header_version() const
{
    return 1;
}

bool AddOffsetsToTxnRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 2;
}

KafkaVersion AddOffsetsToTxnRequest::required_version() const
{
    switch (m_version)
    {
    case 2:
        return V2_7_0_0;
    case 1:
        return V2_0_0_0;
    case 0:
        return V0_11_0_0;
    default:
        return V2_7_0_0;
    }
}