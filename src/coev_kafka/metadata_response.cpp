#include "version.h"
#include "metadata_response.h"
#include "broker.h"

int PartitionMetadata::decode(packet_decoder &pd, int16_t version)
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

    int32_t err;
    pd.getEmptyTaggedFieldArray(err);
    return err;
}

int PartitionMetadata::encode(packet_encoder &pe, int16_t version) const
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

int TopicMetadata::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;
    if (int err = pd.getKError(m_err); err != 0)
    {
        return err;
    }

    if (int err = pd.getString(m_name); err != 0)
    {
        return err;
    }

    if (m_version >= 10)
    {
        std::string_view uuid_bytes;
        if (int err = pd.getRawBytes(16, uuid_bytes); err != 0)
        {
            return err;
        }
        for (size_t i = 0; i < 16; ++i)
        {
            m_uuid.data[i] = uuid_bytes[i];
        }
    }

    if (m_version >= 1)
    {
        if (int err = pd.getBool(m_is_internal); err != 0)
        {
            return err;
        }
    }

    int32_t n;
    if (int err = pd.getArrayLength(n); err != 0)
    {
        return err;
    }
    m_partitions.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        if (int err = m_partitions[i].decode(pd, m_version); err != 0)
        {
            return err;
        }
    }

    if (m_version >= 8)
    {
        if (int err = pd.getInt32(m_topic_authorized_operations); err != 0)
        {
            return err;
        }
    }

    int err;
    pd.getEmptyTaggedFieldArray(err);
    return err;
}

int TopicMetadata::encode(packet_encoder &pe, int16_t version) const
{
    m_version = version;
    pe.putKError(m_err);

    if (int err = pe.putString(m_name); err != 0)
    {
        return err;
    }

    if (m_version >= 10)
    {
        std::string uuid_str(reinterpret_cast<const char *>(m_uuid.data.data()), m_uuid.data.size());
        if (int err = pe.putRawBytes(uuid_str); err != 0)
        {
            return err;
        }
    }

    if (m_version >= 1)
    {
        pe.putBool(m_is_internal);
    }

    if (int err = pe.putArrayLength(static_cast<int32_t>(m_partitions.size())); err != 0)
    {
        return err;
    }
    for (auto &block : m_partitions)
    {
        if (int err = block.encode(pe, m_version); err != 0)
        {
            return err;
        }
    }

    if (m_version >= 8)
    {
        pe.putInt32(m_topic_authorized_operations);
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

void MetadataResponse::set_version(int16_t v)
{
    m_version = v;
}

int MetadataResponse::decode(packet_decoder &pd, int16_t version)
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

    m_brokers.resize(brokerArrayLen);
    for (int32_t i = 0; i < brokerArrayLen; ++i)
    {
        auto _broker = std::make_shared<Broker>();
        if (int err = _broker->decode(pd, version); err != 0)
        {
            return err;
        }
        m_brokers[i] = _broker;
    }

    if (m_version >= 2)
    {
        if (int err = pd.getNullableString(m_cluster_id); err != 0)
        {
            return err;
        }
    }

    if (m_version >= 1)
    {
        if (int err = pd.getInt32(m_controller_id); err != 0)
        {
            return err;
        }
    }

    int32_t topic_array_len;
    if (int err = pd.getArrayLength(topic_array_len); err != 0)
    {
        LOG_CORE("Failed to get topic array length: %d", err);
        return err;
    }

    m_topics.resize(topic_array_len);
    for (int32_t i = 0; i < topic_array_len; ++i)
    {
        if (int err = m_topics[i].decode(pd, version); err != 0)
        {
            LOG_CORE("Failed to decode TopicMetadata %d: %d", i + 1, err);
            return err;
        }
    }

    if (m_version >= 8)
    {
        if (int err = pd.getInt32(m_cluster_authorized_operations); err != 0)
        {
            return err;
        }
    }

    int err;
    pd.getEmptyTaggedFieldArray(err);
    return err;
}

int MetadataResponse::encode(packet_encoder &pe) const
{
    if (m_version >= 3)
    {
        pe.putDurationMs(m_throttle_time);
    }
    int err = ErrNoError;
    if (err = pe.putArrayLength(static_cast<int32_t>(m_brokers.size())); err != 0)
    {
        return err;
    }

    for (auto &broker : m_brokers)
    {
        if (int err = broker->encode(pe, m_version); err != 0)
        {
            return err;
        }
    }

    if (m_version >= 2)
    {
        if (int err = pe.putNullableString(m_cluster_id); err != 0)
        {
            return err;
        }
    }

    if (m_version >= 1)
    {
        pe.putInt32(m_controller_id);
    }

    if (err = pe.putArrayLength(static_cast<int32_t>(m_topics.size())); err != 0)
    {
        return err;
    }
    for (auto &block : m_topics)
    {
        if (int err = block.encode(pe, m_version); err != 0)
        {
            return err;
        }
    }

    if (m_version >= 8)
    {
        pe.putInt32(m_cluster_authorized_operations);
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

void MetadataResponse::add_broker(const std::string &addr, int32_t id)
{
    auto broker = std::make_shared<Broker>();
    broker->m_id = id;
    broker->m_addr = addr;
    m_brokers.push_back(broker);
}

TopicMetadata &MetadataResponse::add_topic(const std::string &topic, int16_t err)
{
    for (auto &tm : m_topics)
    {
        if (tm.m_name == topic)
        {
            tm.m_err = (KError)err;
            return tm;
        }
    }
    m_topics.emplace_back(topic, (KError)err);
    return m_topics.back();
}

int MetadataResponse::add_topic_partition(const std::string &topic, int32_t partition, int32_t brokerID, const std::vector<int32_t> &replicas,
                                          const std::vector<int32_t> &isr, const std::vector<int32_t> &offline,
                                          int err)
{
    auto &tmatch = add_topic(topic, 0);
    for (auto &pm : tmatch.m_partitions)
    {
        if (pm.m_id == partition)
        {
            pm.m_leader = brokerID;
            pm.m_replicas = replicas.empty() ? std::vector<int32_t>{} : replicas;
            pm.m_isr = isr.empty() ? std::vector<int32_t>{} : isr;
            pm.m_offline_replicas = offline.empty() ? std::vector<int32_t>{} : offline;
            pm.m_err = (KError)err;
            return 0;
        }
    }

    PartitionMetadata pmatch;
    pmatch.m_id = partition;
    pmatch.m_leader = brokerID;
    pmatch.m_replicas = replicas.empty() ? std::vector<int32_t>{} : replicas;
    pmatch.m_isr = isr.empty() ? std::vector<int32_t>{} : isr;
    pmatch.m_offline_replicas = offline.empty() ? std::vector<int32_t>{} : offline;
    pmatch.m_err = (KError)err;
    tmatch.m_partitions.push_back(pmatch);
    return 0;
}