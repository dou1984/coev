#include "version.h"
#include "end_txn_response.h"

void EndTxnResponse::set_version(int16_t v)
{
    m_version = v;
}

int EndTxnResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(m_throttle_time);
    pe.putKError(m_err);
    return true;
}

int EndTxnResponse::decode(PDecoder &pd, int16_t)
{
    if (pd.getDurationMs(m_throttle_time) != ErrNoError)
    {
        return ErrDecodeError;
    }
    if (pd.getKError(m_err) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int16_t EndTxnResponse::key() const
{
    return apiKeyEndTxn;
}

int16_t EndTxnResponse::version() const
{
    return m_version;
}

int16_t EndTxnResponse::header_version() const
{
    return 0;
}

bool EndTxnResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 2;
}

KafkaVersion EndTxnResponse::required_version() const
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

std::chrono::milliseconds EndTxnResponse::throttle_time() const
{
    return m_throttle_time;
}