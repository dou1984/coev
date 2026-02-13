#include <cmath>
#include <limits>
#include "version.h"
#include "api_versions.h"
#include "produce_request.h"
#include "length_field.h"

void ProduceRequest::set_version(int16_t v)
{
    m_version = v;
}

int ProduceRequest::encode(packet_encoder &pe) const
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
        for (auto &[pid, partition] : partitions)
        {

            int startOffset = pe.offset();
            pe.putInt32(pid);

            LengthField length_field;
            pe.push(length_field);
            assert(partition);
            if (int err = partition->encode(pe); err != 0)
            {
                return err;
            }
            if (int err = pe.pop(); err != 0)
            {
                return err;
            }
        }
    }

    return 0;
}

int ProduceRequest::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;
    if (version >= 3)
    {
        if (int err = pd.getNullableString(m_transactional_id); err != 0)
        {
            return err;
        }
    }

    int16_t acks;
    if (int err = pd.getInt16(acks); err != 0)
    {
        return err;
    }
    m_acks = static_cast<RequiredAcks>(acks);
    if (int err = pd.getDurationMs(m_timeout); err != 0)
    {
        return err;
    }

    int32_t topic_count;
    if (int err = pd.getArrayLength(topic_count); err != 0)
    {
        return err;
    }
    if (topic_count == 0)
        return 0;

    m_records.clear();
    for (int i = 0; i < topic_count; ++i)
    {
        std::string topic;
        if (int err = pd.getString(topic); err != 0)
        {
            return err;
        }

        int32_t partition_count;
        if (int err = pd.getArrayLength(partition_count); err != 0)
        {
            return err;
        }

        auto &partition_records = m_records[topic];
        for (int j = 0; j < partition_count; ++j)
        {
            int32_t partition;
            if (int err = pd.getInt32(partition); err != 0)
            {
                return err;
            }

            int32_t size;
            if (int err = pd.getInt32(size); err != 0)
            {
                return err;
            }
            std::shared_ptr<packet_decoder> subset;
            if (int err = pd.getSubset(size, subset); err != 0)
            {
                return err;
            }
            auto _record = std::make_shared<Records>();
            if (int err = _record->decode(*subset); err != 0)
            {
                return err;
            }
            partition_records[partition] = _record;
        }
    }

    return 0;
}

int16_t ProduceRequest::key() const
{
    return apiKeyProduce;
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

void ProduceRequest::add_message(const std::string &topic, int32_t partition, std::shared_ptr<Message> msg)
{
    auto record = m_records[topic][partition];
    assert(record);
    if (!record->m_message_set)
    {
        record->m_message_set = std::make_shared<MessageSet>();
    }
    record->m_message_set->add_message(msg);
    record->m_records_type = LegacyRecords;
}

void ProduceRequest::add_set(const std::string &topic, int32_t partition, std::shared_ptr<MessageSet> set)
{
    auto record = m_records[topic][partition];
    assert(record);
    record->m_records_type = LegacyRecords;
    record->m_message_set = set;
}

void ProduceRequest::add_batch(const std::string &topic, int32_t partition, std::shared_ptr<RecordBatch> batch)
{
    auto &record = m_records[topic][partition];
    record->m_records_type = DefaultRecords;
    record->m_record_batch = batch;
}
