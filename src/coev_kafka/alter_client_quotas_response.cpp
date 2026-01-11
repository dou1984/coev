
#include "alter_client_quotas_response.h"

void AlterClientQuotasResponse::set_version(int16_t v)
{
    m_version = v;
}

int AlterClientQuotasResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(m_throttle_time);

    if (int16_t err = pe.putArrayLength(static_cast<int32_t>(m_entries.size())); err != 0)
    {
        return err;
    }
    for (auto &e : m_entries)
    {
        if (int16_t err = e.encode(pe); err != 0)
        {
            return err;
        }
    }

    return 0;
}

int AlterClientQuotasResponse::decode(PDecoder &pd, int16_t version)
{
    if (int16_t err = pd.getDurationMs(m_throttle_time); err != 0)
    {
        return err;
    }

    int32_t entryCount;
    if (int16_t err = pd.getArrayLength(entryCount); err != 0)
    {
        return err;
    }
    if (entryCount > 0)
    {
        m_entries.resize(entryCount);
        for (size_t i = 0; i < static_cast<size_t>(entryCount); ++i)
        {
            AlterClientQuotasEntryResponse e;
            if (int16_t err = e.decode(pd, version); err != 0)
            {
                return err;
            }
            m_entries[i] = e;
        }
    }
    else
    {
        m_entries.clear();
    }

    return 0;
}

int16_t AlterClientQuotasResponse::key() const
{
    return apiKeyAlterClientQuotas;
}

int16_t AlterClientQuotasResponse::version() const
{
    return m_version;
}

int16_t AlterClientQuotasResponse::header_version() const
{
    return 0;
}

bool AlterClientQuotasResponse::is_valid_version() const
{
    return m_version == 0;
}

KafkaVersion AlterClientQuotasResponse::required_version() const
{
    return V2_6_0_0;
}

std::chrono::milliseconds AlterClientQuotasResponse::throttle_time() const
{
    return m_throttle_time;
}

int AlterClientQuotasEntryResponse::encode(PEncoder &pe)
{
    pe.putKError(m_error_code);

    if (int err = pe.putNullableString(m_error_msg); err != 0)
    {
        return err;
    }

    if (int err = pe.putArrayLength(static_cast<int32_t>(m_entity.size())); err != 0)
    {
        return err;
    }
    for (auto &component : m_entity)
    {
        if (int err = component.encode(pe); err != 0)
        {
            return err;
        }
    }

    return 0;
}

int AlterClientQuotasEntryResponse::decode(PDecoder &pd, int16_t version)
{
    int err = ErrNoError;
    if (err = pd.getKError(m_error_code); err != 0)
    {
        return err;
    }

    std::string errMsg;
    if (err = pd.getNullableString(errMsg); err != 0)
    {
        return err;
    }
    m_error_msg = errMsg;

    int32_t componentCount;
    if (err = pd.getArrayLength(componentCount); err != 0)
    {
        return err;
    }
    if (componentCount > 0)
    {
        m_entity.resize(componentCount);
        for (int32_t i = 0; i < componentCount; ++i)
        {
            QuotaEntityComponent component;
            if (int16_t err = component.decode(pd, version); err != 0)
            {
                return err;
            }
            m_entity[i] = component;
        }
    }
    else
    {
        m_entity.clear();
    }

    return 0;
}

int16_t AlterClientQuotasEntryResponse::key() const
{
    return 0;
}
int16_t AlterClientQuotasEntryResponse::version() const
{
    return 0;
}
void AlterClientQuotasEntryResponse::set_version(int16_t version)
{
}
int16_t AlterClientQuotasEntryResponse::header_version() const
{
    return 0;
}
bool AlterClientQuotasEntryResponse::is_valid_version() const
{
    return 0;
}
KafkaVersion AlterClientQuotasEntryResponse::required_version() const
{
    return V2_6_0_0;
}