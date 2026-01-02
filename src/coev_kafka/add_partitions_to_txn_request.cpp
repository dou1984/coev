#include "version.h"
#include "add_partitions_to_txn_request.h"

void AddPartitionsToTxnRequest::setVersion(int16_t v)
{
    Version = v;
}

int AddPartitionsToTxnRequest::encode(PEncoder &pe)
{
    if (pe.putString(TransactionalID) != 0)
    {
        return -1;
    }

    pe.putInt64(ProducerID);
    pe.putInt16(ProducerEpoch);

    if (pe.putArrayLength(static_cast<int32_t>(TopicPartitions.size())) != 0)
    {
        return -1;
    }

    for (auto &kv : TopicPartitions)
    {
        if (pe.putString(kv.first) != 0)
        {
            return -1;
        }
        if (pe.putInt32Array(kv.second) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int AddPartitionsToTxnRequest::decode(PDecoder &pd, int16_t version)
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

    int32_t n;
    if ((err = pd.getArrayLength(n)) != 0)
    {
        return err;
    }

    TopicPartitions.clear();
    for (int32_t i = 0; i < n; ++i)
    {
        std::string topic;
        if ((err = pd.getString(topic)) != 0)
        {
            return err;
        }

        std::vector<int32_t> partitions;
        if ((err = pd.getInt32Array(partitions)) != 0)
        {
            return err;
        }

        TopicPartitions[topic] = std::move(partitions);
    }

    return 0;
}

int16_t AddPartitionsToTxnRequest::key() const
{
    return apiKeyAddPartitionsToTxn;
}

int16_t AddPartitionsToTxnRequest::version() const
{
    return Version;
}

int16_t AddPartitionsToTxnRequest::headerVersion() const
{
    return 1;
}

bool AddPartitionsToTxnRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 2;
}

KafkaVersion AddPartitionsToTxnRequest::requiredVersion() const
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