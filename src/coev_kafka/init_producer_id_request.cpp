#include "version.h"
#include "init_producer_id_request.h"
#include "api_versions.h"

void InitProducerIDRequest::setVersion(int16_t v)
{
    Version = v;
}

int InitProducerIDRequest::encode(PEncoder &pe)
{

    int err = pe.putNullableString(TransactionalID);
    if (err != 0)
        return err;

    pe.putInt32(static_cast<int32_t>(TransactionTimeout.count()));

    if (Version >= 3)
    {
        pe.putInt64(ProducerID);
        pe.putInt16(ProducerEpoch);
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int InitProducerIDRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    int err = pd.getNullableString(TransactionalID);
    if (err != 0)
        return err;

    int32_t timeout;
    err = pd.getInt32(timeout);
    if (err != 0)
        return err;
    TransactionTimeout = std::chrono::milliseconds(timeout);

    if (Version >= 3)
    {
        err = pd.getInt64(ProducerID);
        if (err != 0)
            return err;

        err = pd.getInt16(ProducerEpoch);
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
    return Version;
}

int16_t InitProducerIDRequest::headerVersion() const
{
    return (Version >= 2) ? 2 : 1;
}

bool InitProducerIDRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 4;
}

bool InitProducerIDRequest::isFlexible()
{
    return isFlexibleVersion(Version);
}

bool InitProducerIDRequest::isFlexibleVersion(int16_t ver)
{
    return ver >= 2;
}

KafkaVersion InitProducerIDRequest::requiredVersion() const
{
    switch (Version)
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