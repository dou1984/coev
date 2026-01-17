#include <cmath>
#include <limits>
#include "version.h"
#include "produce_request.h"
#include "length_field.h"

void ProduceRequest::set_version(int16_t v)
{
    m_version = v;
}

int ProduceRequest::encode(packetEncoder &pe)
{
    if (m_version >= 3)
    {
        if (int err = pe.putNullableString(m_transactional_id); err != 0)
        {
            return err;
        }
    }

    pe.putInt16(static_cast<int16_t>(m_acks));
    pe.putDurationMs(m_timeout);


    int64_t totalRecordCount = 0;


    if (int err = pe.putArrayLength(static_cast<int32_t>(m_records.size())); err != 0)
    {
        return err;
    }

    for (auto &[topic, partitions] : m_records)
    {
        if (int err = pe.putString(topic); err != 0)
            return err;
        if (int err = pe.putArrayLength(static_cast<int32_t>(partitions.size())); err != 0)
            return err;

        int64_t topicRecordCount = 0;

        for (auto &[partitionId, recs] : partitions)
        {
            int startOffset = pe.offset();
            pe.putInt32(partitionId);

            auto lengthField = std::make_shared<LengthField>();
            pe.push(lengthField);
            if (int err = recs->encode(pe); err != 0)
                return err;
            if (int err = pe.pop(); err != 0)
                return err;
        }

    }

    return 0;
}

int ProduceRequest::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;

    if (version >= 3)
    {
        if (int err = pd.getNullableString(m_transactional_id); err != 0)
            return err;
    }

    int16_t acks;
    if (int err = pd.getInt16(acks); err != 0)
        return err;
    m_acks = static_cast<RequiredAcks>(acks);

    if (int err = pd.getDurationMs(m_timeout); err != 0)
        return err;

    int32_t topicCount;
    if (int err = pd.getArrayLength(topicCount); err != 0)
        return err;
    if (topicCount == 0)
        return 0;

    m_records.clear();
    for (int i = 0; i < topicCount; ++i)
    {
        std::string topic;
        if (int err = pd.getString(topic); err != 0)
            return err;

        int32_t partitionCount;
        if (int err = pd.getArrayLength(partitionCount); err != 0)
            return err;

        auto &partMap = m_records[topic];
        for (int j = 0; j < partitionCount; ++j)
        {
            int32_t partition;
            if (int err = pd.getInt32(partition); err != 0)
                return err;

            int32_t size;
            if (int err = pd.getInt32(size); err != 0)
                return err;

            std::shared_ptr<packetDecoder> subset;
            if (int err = pd.getSubset(size, subset); err != 0)
                return err;

            auto recs = std::make_shared<Records>();
            if (int err = recs->decode(*subset); err != 0)
                return err;
            partMap[partition] = std::move(recs);
        }
    }

    return 0;
}

int16_t ProduceRequest::key() const
{
    return 0; // apiKeyProduce = 0
}

int16_t ProduceRequest::version() const
{
    return m_version;
}

int16_t ProduceRequest::header_version() const
{
    return 1;
}

bool ProduceRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 9;
}

KafkaVersion ProduceRequest::required_version() const
{
    switch (m_version)
    {
    case 7:
        return V2_1_0_0;
    case 6:
        return V2_0_0_0;
    case 5:
    case 4:
        return V1_0_0_0;
    case 3:
        return V0_11_0_0;
    case 2:
        return V0_10_0_0;
    case 1:
        return V0_9_0_0;
    case 0:
        return V0_8_2_0;
    default:
        return V2_1_0_0;
    }
}

void ProduceRequest::EnsureRecords(const std::string &topic, int32_t partition)
{
}

void ProduceRequest::AddMessage(const std::string &topic, int32_t partition, std::shared_ptr<Message> msg)
{

    if (m_records[topic][partition] == nullptr)
    {
        auto msgSet = std::make_shared<MessageSet>();
        msgSet->addMessage(msg);
        m_records[topic][partition] = Records::NewLegacyRecords(msgSet);
    }
    else
    {
        m_records[topic][partition]->m_msg_set->addMessage(msg);
    }
}

void ProduceRequest::AddSet(const std::string &topic, int32_t partition, std::shared_ptr<MessageSet> set)
{
    EnsureRecords(topic, partition);
    m_records[topic][partition] = Records::NewLegacyRecords(set);
}

void ProduceRequest::AddBatch(const std::string &topic, int32_t partition, std::shared_ptr<RecordBatch> batch)
{
    EnsureRecords(topic, partition);
    m_records[topic][partition] = Records::NewDefaultRecords(batch);
}
