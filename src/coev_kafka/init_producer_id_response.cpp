#include "version.h"
#include "init_producer_id_response.h"

void InitProducerIDResponse::setVersion(int16_t v)
{
    Version = v;
}

int InitProducerIDResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(ThrottleTime);
    pe.putKError(Err);
    pe.putInt64(ProducerID);
    pe.putInt16(ProducerEpoch);
    pe.putEmptyTaggedFieldArray();
    return 0;
}

int InitProducerIDResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    int err = pd.getDurationMs(ThrottleTime);
    if (err != 0)
        return err;

    err = pd.getKError(Err);
    if (err != 0)
        return err;

    err = pd.getInt64(ProducerID);
    if (err != 0)
        return err;

    err = pd.getInt16(ProducerEpoch);
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
    return Version;
}

int16_t InitProducerIDResponse::headerVersion() const
{
    return (Version >= 2) ? 1 : 0;
}

bool InitProducerIDResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 4;
}

bool InitProducerIDResponse::isFlexible()
{
    return isFlexibleVersion(Version);
}

bool InitProducerIDResponse::isFlexibleVersion(int16_t ver)
{
    return ver >= 2;
}

KafkaVersion InitProducerIDResponse::requiredVersion() const
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
    default:
        return V0_11_0_0;
    }
}

std::chrono::milliseconds InitProducerIDResponse::throttleTime() const
{
    return ThrottleTime;
}