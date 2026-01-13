#include "add_offsets_to_txn_response.h"

void AddOffsetsToTxnResponse::set_version(int16_t v)
{
    m_version = v;
}

int AddOffsetsToTxnResponse::encode(packetEncoder &pe)
{

    pe.putDurationMs(m_throttle_time);

    pe.putKError(m_err);
    return 0;
}

int AddOffsetsToTxnResponse::decode(packetDecoder &pd, int16_t version)
{
    int err;
    if ((err = pd.getDurationMs(m_throttle_time)) != 0)
    {
        return err;
    }

    if ((err = pd.getKError(m_err)) != 0)
    {
        return err;
    }

    return 0;
}

int16_t AddOffsetsToTxnResponse::key() const
{
    return apiKeyAddOffsetsToTxn;
}

int16_t AddOffsetsToTxnResponse::version() const
{
    return m_version;
}

int16_t AddOffsetsToTxnResponse::header_version() const
{
    return 0;
}

bool AddOffsetsToTxnResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 2;
}

KafkaVersion AddOffsetsToTxnResponse::required_version() const
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

std::chrono::milliseconds AddOffsetsToTxnResponse::throttle_time() const
{
    return m_throttle_time;
}