#include "version.h"
#include "init_producer_id_request.h"
#include "api_versions.h"

void InitProducerIDRequest::set_version(int16_t v)
{
    m_version = v;
}

int InitProducerIDRequest::encode(packetEncoder &pe)
{

    int err = pe.putNullableString(m_transactional_id);
    if (err != 0)
        return err;

    pe.putInt32(static_cast<int32_t>(m_transaction_timeout.count()));

    if (m_version >= 3)
    {
        pe.putInt64(m_producer_id);
        pe.putInt16(m_producer_epoch);
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int InitProducerIDRequest::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;

    int err = pd.getNullableString(m_transactional_id);
    if (err != 0)
        return err;

    int32_t timeout;
    err = pd.getInt32(timeout);
    if (err != 0)
        return err;
    m_transaction_timeout = std::chrono::milliseconds(timeout);

    if (m_version >= 3)
    {
        err = pd.getInt64(m_producer_id);
        if (err != 0)
            return err;

        err = pd.getInt16(m_producer_epoch);
        if (err != 0)
            return err;
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t InitProducerIDRequest::key() const
{
    return apiKeyInitProducerId;
}

int16_t InitProducerIDRequest::version() const
{
    return m_version;
}

int16_t InitProducerIDRequest::header_version() const
{
    return (m_version >= 2) ? 2 : 1;
}

bool InitProducerIDRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 4;
}

bool InitProducerIDRequest::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool InitProducerIDRequest::is_flexible_version(int16_t ver) const
{
    return ver >= 2;
}

KafkaVersion InitProducerIDRequest::required_version() const
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
    case 0:
        return V0_11_0_0;
    default:
        return V2_7_0_0;
    }
}