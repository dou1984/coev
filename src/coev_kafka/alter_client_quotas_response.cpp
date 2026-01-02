
#include "alter_client_quotas_response.h"

void AlterClientQuotasResponse::setVersion(int16_t v)
{
    Version = v;
}

int AlterClientQuotasResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(ThrottleTime);

    if (int16_t err = pe.putArrayLength(static_cast<int32_t>(Entries.size())); err != 0)
    {
        return err;
    }
    for (auto &e : Entries)
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
    if (int16_t err = pd.getDurationMs(ThrottleTime); err != 0)
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
        Entries.resize(entryCount);
        for (size_t i = 0; i < static_cast<size_t>(entryCount); ++i)
        {
            AlterClientQuotasEntryResponse e;
            if (int16_t err = e.decode(pd, version); err != 0)
            {
                return err;
            }
            Entries[i] = e;
        }
    }
    else
    {
        Entries.clear();
    }

    return 0;
}

int16_t AlterClientQuotasResponse::key() const
{
    return apiKeyAlterClientQuotas;
}

int16_t AlterClientQuotasResponse::version() const
{
    return Version;
}

int16_t AlterClientQuotasResponse::headerVersion() const
{
    return 0;
}

bool AlterClientQuotasResponse::isValidVersion() const
{
    return Version == 0;
}

KafkaVersion AlterClientQuotasResponse::requiredVersion() const
{
    return V2_6_0_0;
}

std::chrono::milliseconds AlterClientQuotasResponse::throttleTime() const
{
    return ThrottleTime;
}

int AlterClientQuotasEntryResponse::encode(PEncoder &pe)
{
    pe.putKError(ErrorCode);

    if (int err = pe.putNullableString(ErrorMsg); err != 0)
    {
        return err;
    }

    if (int err = pe.putArrayLength(static_cast<int32_t>(Entity.size())); err != 0)
    {
        return err;
    }
    for (auto &component : Entity)
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
    if (err = pd.getKError(ErrorCode); err != 0)
    {
        return err;
    }

    std::string errMsg;
    if (err = pd.getNullableString(errMsg); err != 0)
    {
        return err;
    }
    ErrorMsg = errMsg;

    int32_t componentCount;
    if (err = pd.getArrayLength(componentCount); err != 0)
    {
        return err;
    }
    if (componentCount > 0)
    {
        Entity.resize(componentCount);
        for (int32_t i = 0; i < componentCount; ++i)
        {
            QuotaEntityComponent component;
            if (int16_t err = component.decode(pd, version); err != 0)
            {
                return err;
            }
            Entity[i] = component;
        }
    }
    else
    {
        Entity.clear();
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
void AlterClientQuotasEntryResponse::setVersion(int16_t version)
{
}
int16_t AlterClientQuotasEntryResponse::headerVersion() const
{
    return 0;
}
bool AlterClientQuotasEntryResponse::isValidVersion() const
{
    return 0;
}
KafkaVersion AlterClientQuotasEntryResponse::requiredVersion() const
{
    return V2_6_0_0;
}