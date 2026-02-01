#include "version.h"
#include "add_partitions_to_txn_request.h"

void AddPartitionsToTxnRequest::set_version(int16_t v)
{
    m_version = v;
}

int AddPartitionsToTxnRequest::encode(packet_encoder &pe) const
{
    if (pe.putString(m_transactional_id) != 0)
    {
        return -1;
    }

    pe.putInt64(m_producer_id);
    pe.putInt16(m_producer_epoch);

    if (pe.putArrayLength(static_cast<int32_t>(m_topic_partitions.size())) != 0)
    {
        return -1;
    }

    for (auto &kv : m_topic_partitions)
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

int AddPartitionsToTxnRequest::decode(packet_decoder &pd, int16_t version)
{
    int err;
    if ((err = pd.getString(m_transactional_id)) != 0)
    {
        return err;
    }
    if ((err = pd.getInt64(m_producer_id)) != 0)
    {
        return err;
    }
    if ((err = pd.getInt16(m_producer_epoch)) != 0)
    {
        return err;
    }

    int32_t n;
    if ((err = pd.getArrayLength(n)) != 0)
    {
        return err;
    }

    m_topic_partitions.clear();
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

        m_topic_partitions[topic] = std::move(partitions);
    }

    return 0;
}

int16_t AddPartitionsToTxnRequest::key() const
{
    return apiKeyAddPartitionsToTxn;
}

int16_t AddPartitionsToTxnRequest::version() const
{
    return m_version;
}

int16_t AddPartitionsToTxnRequest::header_version() const
{
    return 1;
}

bool AddPartitionsToTxnRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 2;
}

KafkaVersion AddPartitionsToTxnRequest::required_version() const
{
    switch (m_version)
    {
    case 2:
        return V2_7_0_0;
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}