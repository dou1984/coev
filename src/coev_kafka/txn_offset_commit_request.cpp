#include "version.h"
#include "txn_offset_commit_request.h"
#include "packet_encoder.h"
#include "packet_decoder.h"

void TxnOffsetCommitRequest::setVersion(int16_t v)
{
    Version = v;
}

int TxnOffsetCommitRequest::encode(PEncoder &pe)
{
    pe.putString(TransactionalID);
    pe.putString(GroupID);
    pe.putInt64(ProducerID);
    pe.putInt16(ProducerEpoch);

    pe.putArrayLength(static_cast<int32_t>(Topics.size()));
    for (auto &topicPair : Topics)
    {
        const std::string &topic = topicPair.first;
        auto &partitions = topicPair.second;
        pe.putString(topic);
        pe.putArrayLength(static_cast<int32_t>(partitions.size()));
        for (auto &partition : partitions)
        {
            partition->encode(pe, Version);
        }
    }
    return 0;
}

int TxnOffsetCommitRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    pd.getString(TransactionalID);
    pd.getString(GroupID);
    pd.getInt64(ProducerID);
    pd.getInt16(ProducerEpoch);

    int32_t n;
    pd.getArrayLength(n);
    Topics.clear();
    int err = 0;
    for (int i = 0; i < n; ++i)
    {
        std::string topic;
        err = pd.getString(topic);
        if (err)
            return err;
        int32_t m;
        err = pd.getArrayLength(m);
        if (err)
            return err;
        std::vector<std::shared_ptr<PartitionOffsetMetadata>> partitions(m);
        for (int j = 0; j < m; ++j)
        {
            auto partitionOffsetMetadata = std::make_shared<PartitionOffsetMetadata>();
            partitionOffsetMetadata->decode(pd, version);
            partitions[j] = partitionOffsetMetadata;
        }
        Topics[topic] = std::move(partitions);
    }
    return 0;
}

int16_t TxnOffsetCommitRequest::key() const
{
    return apiKeyTxnOffsetCommit;
}

int16_t TxnOffsetCommitRequest::version() const
{
    return Version;
}

int16_t TxnOffsetCommitRequest::headerVersion() const
{
    return 1;
}

bool TxnOffsetCommitRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 2;
}

KafkaVersion TxnOffsetCommitRequest::requiredVersion() const
{
    switch (Version)
    {
    case 2:
        return V2_1_0_0;
    case 1:
        return V2_0_0_0;
    case 0:
        return V0_11_0_0;
    default:
        return V2_1_0_0;
    }
}
