#include "version.h"
#include "end_txn_request.h"

void EndTxnRequest::set_version(int16_t v)
{
    m_version = v;
}

int EndTxnRequest::encode(packet_encoder &pe) const
{
    if (pe.putString(m_transactional_id) != ErrNoError)
    {
        return ErrEncodeError;
    }
    pe.putInt64(m_producer_id);
    pe.putInt16(m_producer_epoch);
    pe.putBool(m_transaction_result);
    return ErrNoError;
}

int EndTxnRequest::decode(packet_decoder &pd, int16_t /*version*/)
{
    if (pd.getString(m_transactional_id) != ErrNoError)
    {
        return ErrDecodeError;
    }
    if (pd.getInt64(m_producer_id) != ErrNoError)
    {
        return ErrDecodeError;
    }
    if (pd.getInt16(m_producer_epoch) != ErrNoError)
    {
        return ErrDecodeError;
    }
    if (pd.getBool(m_transaction_result) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int16_t EndTxnRequest::key() const
{
    return apiKeyEndTxn;
}

int16_t EndTxnRequest::version() const
{
    return m_version;
}

int16_t EndTxnRequest::header_version() const
{
    return 1;
}

bool EndTxnRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 2;
}

KafkaVersion EndTxnRequest::required_version()  const
{
    switch (m_version)
    {
    case 2:
        return V2_7_0_0;
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}