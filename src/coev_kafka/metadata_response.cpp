#include "version.h"
#include "metadata_response.h"
#include "broker.h"

int PartitionMetadata::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    if (int err = pd.getKError(Err); err != 0)
    {
        return err;
    }

    if (int err = pd.getInt32(ID); err != 0)
    {
        return err;
    }

    if (int err = pd.getInt32(Leader); err != 0)
    {
        return err;
    }

    if (Version >= 7)
    {
        if (int err = pd.getInt32(LeaderEpoch); err != 0)
        {
            return err;
        }
    }

    if (int err = pd.getInt32Array(Replicas); err != 0)
    {
        return err;
    }

    if (int err = pd.getInt32Array(Isr); err != 0)
    {
        return err;
    }

    if (Version >= 5)
    {
        if (int err = pd.getInt32Array(OfflineReplicas); err != 0)
        {
            return err;
        }
    }

    int err;
    pd.getEmptyTaggedFieldArray(err);
    return err;
}

int PartitionMetadata::encode(PEncoder &pe, int16_t version)
{
    Version = version;
    pe.putKError(Err);

    pe.putInt32(ID);

    pe.putInt32(Leader);

    if (Version >= 7)
    {
        pe.putInt32(LeaderEpoch);
    }

    if (int err = pe.putInt32Array(Replicas); err != 0)
    {
        return err;
    }

    if (int err = pe.putInt32Array(Isr); err != 0)
    {
        return err;
    }

    if (Version >= 5)
    {
        if (int err = pe.putInt32Array(OfflineReplicas); err != 0)
        {
            return err;
        }
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int TopicMetadata::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    if (int err = pd.getKError(Err); err != 0)
    {
        return err;
    }

    if (int err = pd.getString(Name); err != 0)
    {
        return err;
    }

    if (Version >= 10)
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

    if (Version >= 1)
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
        if (int err = block->decode(pd, Version); err != 0)
        {
            return err;
        }
        Partitions[i] = block;
    }

    if (Version >= 8)
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

int TopicMetadata::encode(PEncoder &pe, int16_t version)
{
    Version = version;
    pe.putKError(Err);

    if (int err = pe.putString(Name); err != 0)
    {
        return err;
    }

    if (Version >= 10)
    {
        std::string uuidBytes(Uuid_.data.begin(), Uuid_.data.end());
        if (int err = pe.putRawBytes(uuidBytes); err != 0)
        {
            return err;
        }
    }

    if (Version >= 1)
    {
        pe.putBool(IsInternal);
    }

    if (int err = pe.putArrayLength(static_cast<int32_t>(Partitions.size())); err != 0)
    {
        return err;
    }
    for (auto &block : Partitions)
    {
        if (int err = block->encode(pe, Version); err != 0)
        {
            return err;
        }
    }

    if (Version >= 8)
    {
        pe.putInt32(TopicAuthorizedOperations);
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

void MetadataResponse::setVersion(int16_t v)
{
    Version = v;
}

int MetadataResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    if (Version >= 3)
    {
        if (int err = pd.getDurationMs(ThrottleTime); err != 0)
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

    if (Version >= 2)
    {
        if (int err = pd.getNullableString(ClusterID); err != 0)
        {
            return err;
        }
    }

    if (Version >= 1)
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

    if (Version >= 8)
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

int MetadataResponse::encode(PEncoder &pe)
{
    if (Version >= 3)
    {
        pe.putDurationMs(ThrottleTime);
    }
    int err = ErrNoError;
    if (err = pe.putArrayLength(static_cast<int32_t>(Brokers.size())); err != 0)
    {
        return err;
    }

    for (auto &broker : Brokers)
    {
        if (int err = broker->encode(pe, Version); err != 0)
        {
            return err;
        }
    }

    if (Version >= 2)
    {
        if (int err = pe.putNullableString(ClusterID); err != 0)
        {
            return err;
        }
    }

    if (Version >= 1)
    {
        pe.putInt32(ControllerID);
    }

    if (err = pe.putArrayLength(static_cast<int32_t>(Topics.size())); err != 0)
    {
        return err;
    }
    for (auto &block : Topics)
    {
        if (int err = block->encode(pe, Version); err != 0)
        {
            return err;
        }
    }

    if (Version >= 8)
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
    return Version;
}

int16_t MetadataResponse::headerVersion() const
{
    if (Version < 9)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

bool MetadataResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 10;
}

bool MetadataResponse::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool MetadataResponse::isFlexibleVersion(int16_t version) const
{
    return version >= 9;
}

KafkaVersion MetadataResponse::requiredVersion() const
{
    switch (Version)
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

std::chrono::milliseconds MetadataResponse::throttleTime() const
{
    return std::chrono::milliseconds(ThrottleTime);
}

void MetadataResponse::AddBroker(const std::string &addr, int32_t id)
{
    auto broker = std::make_shared<Broker>();
    broker->m_ID = id;
    broker->m_Addr = addr;
    Brokers.push_back(broker);
}

std::shared_ptr<TopicMetadata> MetadataResponse::AddTopic(const std::string &topic, int16_t err)
{
    for (auto &tm : Topics)
    {
        if (tm->Name == topic)
        {
            tm->Err = (KError)err;
            return tm;
        }
    }

    auto tmatch = std::make_shared<TopicMetadata>();
    tmatch->Name = topic;
    tmatch->Err = (KError)err;
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
        if (pm->ID == partition)
        {
            pm->Leader = brokerID;
            pm->Replicas = replicas.empty() ? std::vector<int32_t>{} : replicas;
            pm->Isr = isr.empty() ? std::vector<int32_t>{} : isr;
            pm->OfflineReplicas = offline.empty() ? std::vector<int32_t>{} : offline;
            pm->Err = (KError)err;
            return 0;
        }
    }

    auto pmatch = std::make_shared<PartitionMetadata>();
    pmatch->ID = partition;
    pmatch->Leader = brokerID;
    pmatch->Replicas = replicas.empty() ? std::vector<int32_t>{} : replicas;
    pmatch->Isr = isr.empty() ? std::vector<int32_t>{} : isr;
    pmatch->OfflineReplicas = offline.empty() ? std::vector<int32_t>{} : offline;
    pmatch->Err = (KError)err;
    tmatch->Partitions.push_back(pmatch);
    return 0;
}