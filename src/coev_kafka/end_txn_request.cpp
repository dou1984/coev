#include "version.h"
#include "end_txn_request.h"

void EndTxnRequest::setVersion(int16_t v)
{
    Version = v;
}

int EndTxnRequest::encode(PEncoder &pe)
{
    if (!pe.putString(TransactionalID))
    {
        return ErrEncodeError;
    }
    pe.putInt64(ProducerID);
    pe.putInt16(ProducerEpoch);
    pe.putBool(TransactionResult);
    return true;
}

int EndTxnRequest::decode(PDecoder &pd, int16_t /*version*/)
{
    if (pd.getString(TransactionalID) != ErrNoError)
    {
        return ErrDecodeError;
    }
    if (pd.getInt64(ProducerID) != ErrNoError)
    {
        return ErrDecodeError;
    }
    if (pd.getInt16(ProducerEpoch) != ErrNoError)
    {
        return ErrDecodeError;
    }
    if (pd.getBool(TransactionResult) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int16_t EndTxnRequest::key() const
{
    return apiKeyEndTxn;
}

int16_t EndTxnRequest::version() const
{
    return Version;
}

int16_t EndTxnRequest::headerVersion() const
{
    return 1;
}

bool EndTxnRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 2;
}

KafkaVersion EndTxnRequest::requiredVersion() const
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