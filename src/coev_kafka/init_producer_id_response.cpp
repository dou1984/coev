#include "version.h"
#include "init_producer_id_response.h"

void InitProducerIDResponse::set_version(int16_t v)
{
    m_version = v;
}

int InitProducerIDResponse::encode(packetEncoder &pe)
{
    pe.putDurationMs(m_throttle_time);
    pe.putKError(m_err);
    pe.putInt64(m_producer_id);
    pe.putInt16(m_producer_epoch);
    pe.putEmptyTaggedFieldArray();
    return 0;
}

int InitProducerIDResponse::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;

    int err = pd.getDurationMs(m_throttle_time);
    if (err != 0)
        return err;

    err = pd.getKError(m_err);
    if (err != 0)
        return err;

    err = pd.getInt64(m_producer_id);
    if (err != 0)
        return err;

    err = pd.getInt16(m_producer_epoch);
    if (err != 0)
        return err;
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t InitProducerIDResponse::key() const
{
    return apiKeyInitProducerId;
}

int16_t InitProducerIDResponse::version() const
{
    return m_version;
}

int16_t InitProducerIDResponse::header_version() const
{
    return (m_version >= 2) ? 1 : 0;
}

bool InitProducerIDResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 4;
}

bool InitProducerIDResponse::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool InitProducerIDResponse::is_flexible_version(int16_t ver) const
{
    return ver >= 2;
}

KafkaVersion InitProducerIDResponse::required_version() const
{
    switch (m_version)
    {
    case 4:
        return V2_7_0_0;
    case 3:
        return V2_5_0_0;
    case 2:
        return V2_4_0_0;
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}

std::chrono::milliseconds InitProducerIDResponse::throttle_time() const
{
    return m_throttle_time;
}