#include "version.h"
#include "end_txn_response.h"

void EndTxnResponse::setVersion(int16_t v)
{
    Version = v;
}

int EndTxnResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(ThrottleTime);
    pe.putKError(Err);
    return true;
}

int EndTxnResponse::decode(PDecoder &pd, int16_t)
{
    if (pd.getDurationMs(ThrottleTime) != ErrNoError)
    {
        return ErrDecodeError;
    }
    if (pd.getKError(Err) != ErrNoError)
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
    return Version;
}

int16_t EndTxnResponse::headerVersion() const
{
    return 0;
}

bool EndTxnResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 2;
}

KafkaVersion EndTxnResponse::requiredVersion() const
{
    switch (Version)
    {
    case 2:
        return V2_7_0_0;
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}

std::chrono::milliseconds EndTxnResponse::throttleTime() const
{
    return ThrottleTime;
}