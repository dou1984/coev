#include "add_offsets_to_txn_response.h"

void AddOffsetsToTxnResponse::setVersion(int16_t v)
{
    Version = v;
}

int AddOffsetsToTxnResponse::encode(PEncoder &pe)
{

    pe.putDurationMs(ThrottleTime);

    pe.putKError(Err);
    return 0;
}

int AddOffsetsToTxnResponse::decode(PDecoder &pd, int16_t version)
{
    int err;
    if ((err = pd.getDurationMs(ThrottleTime)) != 0)
    {
        return err;
    }

    if ((err = pd.getKError(Err)) != 0)
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
    return Version;
}

int16_t AddOffsetsToTxnResponse::headerVersion() const
{
    return 0;
}

bool AddOffsetsToTxnResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 2;
}

KafkaVersion AddOffsetsToTxnResponse::requiredVersion() const
{
    switch (Version)
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

std::chrono::milliseconds AddOffsetsToTxnResponse::throttleTime() const
{
    return ThrottleTime;
}