#include "version.h"
#include "add_offsets_to_txn_request.h"

void AddOffsetsToTxnRequest::setVersion(int16_t v)
{
    Version = v;
}

int AddOffsetsToTxnRequest::encode(PEncoder &pe)
{
    if (pe.putString(TransactionalID) != 0)
    {
        return -1;
    }

    pe.putInt64(ProducerID);
    pe.putInt16(ProducerEpoch);

    if (pe.putString(GroupID) != 0)
    {
        return -1;
    }

    return 0;
}

int AddOffsetsToTxnRequest::decode(PDecoder &pd, int16_t version)
{
    int err;
    if ((err = pd.getString(TransactionalID)) != 0)
    {
        return err;
    }
    if ((err = pd.getInt64(ProducerID)) != 0)
    {
        return err;
    }
    if ((err = pd.getInt16(ProducerEpoch)) != 0)
    {
        return err;
    }
    if ((err = pd.getString(GroupID)) != 0)
    {
        return err;
    }
    return 0;
}

int16_t AddOffsetsToTxnRequest::key() const
{
    return apiKeyAddOffsetsToTxn;
}

int16_t AddOffsetsToTxnRequest::version() const
{
    return Version;
}

int16_t AddOffsetsToTxnRequest::headerVersion() const
{
    return 1;
}

bool AddOffsetsToTxnRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 2;
}

KafkaVersion AddOffsetsToTxnRequest::requiredVersion() const
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