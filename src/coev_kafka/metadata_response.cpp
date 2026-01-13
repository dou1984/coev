#include "version.h"
#include "metadata_response.h"
#include "broker.h"

int PartitionMetadata::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;
    if (int err = pd.getKError(m_err); err != 0)
    {
        return err;
    }

    if (int err = pd.getInt32(m_id); err != 0)
    {
        return err;
    }

    if (int err = pd.getInt32(m_leader); err != 0)
    {
        return err;
    }

    if (m_version >= 7)
    {
        if (int err = pd.getInt32(m_leader_epoch); err != 0)
        {
            return err;
        }
    }

    if (int err = pd.getInt32Array(m_replicas); err != 0)
    {
        return err;
    }

    if (int err = pd.getInt32Array(m_isr); err != 0)
    {
        return err;
    }

    if (m_version >= 5)
    {
        if (int err = pd.getInt32Array(m_offline_replicas); err != 0)
        {
            return err;
        }
    }

    int err;
    pd.getEmptyTaggedFieldArray(err);
    return err;
}

int PartitionMetadata::encode(packetEncoder &pe, int16_t version)
{
    m_version = version;
    pe.putKError(m_err);

    pe.putInt32(m_id);

    pe.putInt32(m_leader);

    if (m_version >= 7)
    {
        pe.putInt32(m_leader_epoch);
    }

    if (int err = pe.putInt32Array(m_replicas); err != 0)
    {
        return err;
    }

    if (int err = pe.putInt32Array(m_isr); err != 0)
    {
        return err;
    }

    if (m_version >= 5)
    {
        if (int err = pe.putInt32Array(m_offline_replicas); err != 0)
        {
            return err;
        }
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int TopicMetadata::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;
    if (int err = pd.getKError(m_err); err != 0)
    {
        return err;
    }

    if (int err = pd.getString(Name); err != 0)
    {
        return err;
    }

    if (m_version >= 10)
    {
        std::string uuidBytes;
        if (int err = pd.getRawBytes(16, uuidBytes); err != 0)
        {
            return err;
        }
        for (size_t i = 0; i < 16; ++i)
        {
            Uuid_.data[i] = uuidBytes[i];
        }
    }

    if (m_version >= 1)
    {
        if (int err = pd.getBool(IsInternal); err != 0)
        {
            return err;
        }
    }

    int32_t n;
    if (int err = pd.getArrayLength(n); err != 0)
    {
        return err;
    }
    Partitions.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        auto block = std::make_shared<PartitionMetadata>();
        if (int err = block->decode(pd, m_version); err != 0)
        {
            return err;
        }
        Partitions[i] = block;
    }

    if (m_version >= 8)
    {
        if (int err = pd.getInt32(TopicAuthorizedOperations); err != 0)
        {
            return err;
        }
    }

    int err;
    pd.getEmptyTaggedFieldArray(err);
    return err;
}

int TopicMetadata::encode(packetEncoder &pe, int16_t version)
{
    m_version = version;
    pe.putKError(m_err);

    if (int err = pe.putString(Name); err != 0)
    {
        return err;
    }

    if (m_version >= 10)
    {
        std::string uuidBytes(Uuid_.data.begin(), Uuid_.data.end());
        if (int err = pe.putRawBytes(uuidBytes); err != 0)
        {
            return err;
        }
    }

    if (m_version >= 1)
    {
        pe.putBool(IsInternal);
    }

    if (int err = pe.putArrayLength(static_cast<int32_t>(Partitions.size())); err != 0)
    {
        return err;
    }
    for (auto &block : Partitions)
    {
        if (int err = block->encode(pe, m_version); err != 0)
        {
            return err;
        }
    }

    if (m_version >= 8)
    {
        pe.putInt32(TopicAuthorizedOperations);
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

void MetadataResponse::set_version(int16_t v)
{
    m_version = v;
}

int MetadataResponse::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;
    if (m_version >= 3)
    {
        if (int err = pd.getDurationMs(m_throttle_time); err != 0)
        {
            return err;
        }
    }

    int32_t brokerArrayLen;
    if (int err = pd.getArrayLength(brokerArrayLen); err != 0)
    {
        return err;
    }

    Brokers.resize(brokerArrayLen);
    for (int32_t i = 0; i < brokerArrayLen; ++i)
    {
        auto broker = std::make_shared<Broker>();
        if (int err = broker->decode(pd, version); err != 0)
        {
            return err;
        }
        Brokers[i] = broker;
    }

    if (m_version >= 2)
    {
        if (int err = pd.getNullableString(ClusterID); err != 0)
        {
            return err;
        }
    }

    if (m_version >= 1)
    {
        if (int err = pd.getInt32(ControllerID); err != 0)
        {
            return err;
        }
    }

    int32_t topicArrayLen;
    if (int err = pd.getArrayLength(topicArrayLen); err != 0)
    {
        return err;
    }

    Topics.resize(topicArrayLen);
    for (int32_t i = 0; i < topicArrayLen; ++i)
    {
        auto topic = std::make_shared<TopicMetadata>();
        if (int err = topic->decode(pd, version); err != 0)
        {
            return err;
        }
        Topics[i] = topic;
    }

    if (m_version >= 8)
    {
        if (int err = pd.getInt32(ClusterAuthorizedOperations); err != 0)
        {
            return err;
        }
    }

    int err;
    pd.getEmptyTaggedFieldArray(err);
    return err;
}

int MetadataResponse::encode(packetEncoder &pe)
{
    if (m_version >= 3)
    {
        pe.putDurationMs(m_throttle_time);
    }
    int err = ErrNoError;
    if (err = pe.putArrayLength(static_cast<int32_t>(Brokers.size())); err != 0)
    {
        return err;
    }

    for (auto &broker : Brokers)
    {
        if (int err = broker->encode(pe, m_version); err != 0)
        {
            return err;
        }
    }

    if (m_version >= 2)
    {
        if (int err = pe.putNullableString(ClusterID); err != 0)
        {
            return err;
        }
    }

    if (m_version >= 1)
    {
        pe.putInt32(ControllerID);
    }

    if (err = pe.putArrayLength(static_cast<int32_t>(Topics.size())); err != 0)
    {
        return err;
    }
    for (auto &block : Topics)
    {
        if (int err = block->encode(pe, m_version); err != 0)
        {
            return err;
        }
    }

    if (m_version >= 8)
    {
        pe.putInt32(ClusterAuthorizedOperations);
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int16_t MetadataResponse::key() const
{
    return apiKeyMetadata;
}

int16_t MetadataResponse::version() const
{
    return m_version;
}

int16_t MetadataResponse::header_version() const
{
    if (m_version < 9)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

bool MetadataResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 10;
}

bool MetadataResponse::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool MetadataResponse::is_flexible_version(int16_t version) const
{
    return version >= 9;
}

KafkaVersion MetadataResponse::required_version() const
{
    switch (m_version)
    {
    case 10:
        return V2_8_0_0;
    case 9:
        return V2_4_0_0;
    case 8:
        return V2_3_0_0;
    case 7:
        return V2_1_0_0;
    case 6:
        return V2_0_0_0;
    case 5:
        return V1_0_0_0;
    case 3:
    case 4:
        return V0_11_0_0;
    case 2:
        return V0_10_1_0;
    case 1:
        return V0_10_0_0;
    case 0:
        return V0_8_2_0;
    default:
        return V2_8_0_0;
    }
}

std::chrono::milliseconds MetadataResponse::throttle_time() const
{
    return std::chrono::milliseconds(m_throttle_time);
}

void MetadataResponse::AddBroker(const std::string &addr, int32_t id)
{
    auto broker = std::make_shared<Broker>();
    broker->m_id = id;
    broker->m_addr = addr;
    Brokers.push_back(broker);
}

std::shared_ptr<TopicMetadata> MetadataResponse::AddTopic(const std::string &topic, int16_t err)
{
    for (auto &tm : Topics)
    {
        if (tm->Name == topic)
        {
            tm->m_err = (KError)err;
            return tm;
        }
    }

    auto tmatch = std::make_shared<TopicMetadata>();
    tmatch->Name = topic;
    tmatch->m_err = (KError)err;
    Topics.push_back(tmatch);
    return tmatch;
}

int MetadataResponse::AddTopicPartition(const std::string &topic, int32_t partition, int32_t brokerID, const std::vector<int32_t> &replicas,
                                        const std::vector<int32_t> &isr, const std::vector<int32_t> &offline,
                                        int err)
{
    auto tmatch = AddTopic(topic, 0);
    for (auto &pm : tmatch->Partitions)
    {
        if (pm->m_id == partition)
        {
            pm->m_leader = brokerID;
            pm->m_replicas = replicas.empty() ? std::vector<int32_t>{} : replicas;
            pm->m_isr = isr.empty() ? std::vector<int32_t>{} : isr;
            pm->m_offline_replicas = offline.empty() ? std::vector<int32_t>{} : offline;
            pm->m_err = (KError)err;
            return 0;
        }
    }

    auto pmatch = std::make_shared<PartitionMetadata>();
    pmatch->m_id = partition;
    pmatch->m_leader = brokerID;
    pmatch->m_replicas = replicas.empty() ? std::vector<int32_t>{} : replicas;
    pmatch->m_isr = isr.empty() ? std::vector<int32_t>{} : isr;
    pmatch->m_offline_replicas = offline.empty() ? std::vector<int32_t>{} : offline;
    pmatch->m_err = (KError)err;
    tmatch->Partitions.push_back(pmatch);
    return 0;
}