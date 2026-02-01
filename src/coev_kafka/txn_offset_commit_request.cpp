#include "version.h"
#include "txn_offset_commit_request.h"
#include "packet_encoder.h"
#include "packet_decoder.h"

void TxnOffsetCommitRequest::set_version(int16_t v)
{
    m_version = v;
}

int TxnOffsetCommitRequest::encode(packet_encoder &pe) const
{
    pe.putString(m_transactional_id);
    pe.putString(m_group_id);
    pe.putInt64(m_producer_id);
    pe.putInt16(m_producer_epoch);

    pe.putArrayLength(static_cast<int32_t>(m_topics.size()));
    for (auto &topicPair : m_topics)
    {
        const std::string &topic = topicPair.first;
        auto &partitions = topicPair.second;
        pe.putString(topic);
        pe.putArrayLength(static_cast<int32_t>(partitions.size()));
        for (auto &partition : partitions)
        {
            partition->encode(pe, m_version);
        }
    }
    return 0;
}

int TxnOffsetCommitRequest::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;
    pd.getString(m_transactional_id);
    pd.getString(m_group_id);
    pd.getInt64(m_producer_id);
    pd.getInt16(m_producer_epoch);

    int32_t n;
    pd.getArrayLength(n);
    m_topics.clear();
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
        m_topics[topic] = std::move(partitions);
    }
    return 0;
}

int16_t TxnOffsetCommitRequest::key() const
{
    return apiKeyTxnOffsetCommit;
}

int16_t TxnOffsetCommitRequest::version() const
{
    return m_version;
}

int16_t TxnOffsetCommitRequest::header_version() const
{
    return 1;
}

bool TxnOffsetCommitRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 2;
}

KafkaVersion TxnOffsetCommitRequest::required_version() const
{
    switch (m_version)
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
