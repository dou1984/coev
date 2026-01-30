#include <algorithm>
#include "version.h"
#include "fetch_response.h"
#include "length_field.h"
#include "real_encoder.h"
AbortedTransaction::AbortedTransaction() : m_producer_id(0), m_first_offset(0)
{
}

int AbortedTransaction::decode(packetDecoder &pd)
{
    int err = 0;
    if ((err = pd.getInt64(m_producer_id)) != 0)
        return err;
    if ((err = pd.getInt64(m_first_offset)) != 0)
        return err;
    return 0;
}

int AbortedTransaction::encode(packetEncoder &pe)
{
    pe.putInt64(m_producer_id);
    pe.putInt64(m_first_offset);
    return 0;
}

FetchResponseBlock::FetchResponseBlock()
    : m_err(static_cast<KError>(0)), m_high_water_mark_offset(0), m_last_stable_offset(0), m_log_start_offset(0),
      m_preferred_read_replica(-1), m_partial(false)
{
}

int FetchResponseBlock::decode(packetDecoder &pd, int16_t version)
{
    int err = 0;
    if ((err = pd.getKError(m_err)) != 0)
    {
        return err;
    }
    if ((err = pd.getInt64(m_high_water_mark_offset)) != 0)
    {
        return err;
    }
    if (version >= 4)
    {
        if ((err = pd.getInt64(m_last_stable_offset)) != 0)
        {
            return err;
        }
        if (version >= 5)
        {
            if ((err = pd.getInt64(m_log_start_offset)) != 0)
            {
                return err;
            }
        }
        int32_t numTransact;
        if ((err = pd.getArrayLength(numTransact)) != 0)
        {
            return err;
        }
        if (numTransact >= 0)
        {
            m_aborted_transactions.resize(numTransact);
            for (int i = 0; i < numTransact; ++i)
            {

                if ((err = m_aborted_transactions[i].decode(pd)) != 0)
                {
                    return err;
                }
            }
        }
    }

    if (version >= 11)
    {
        if ((err = pd.getInt32(m_preferred_read_replica)) != 0)
            return err;
    }
    else
    {
        m_preferred_read_replica = -1;
    }

    int32_t recordsSize;
    if ((err = pd.getInt32(recordsSize)) != 0)
    {
        return err;
    }

    std::shared_ptr<packetDecoder> records_decoder;
    if ((err = pd.getSubset(recordsSize, records_decoder)) != 0)
    {
        return err;
    }

    m_records_set.clear();
    while (records_decoder->remaining() > 0)
    {
        Records records;
        err = records.decode(*records_decoder);
        bool isInsufficientData = (err == ErrInsufficientData);

        if (err != 0 && !isInsufficientData)
        {
            return err;
        }

        if (isInsufficientData && m_records_set.empty())
        {
            m_partial = true;
            break;
        }

        int64_t nextOffset;
        if ((err = records.next_offset(nextOffset)) != 0)
        {
            return err;
        }
        m_records_next_offset = nextOffset;

        bool partial = false;
        if ((err = records.is_partial(partial)) != 0)
        {
            return err;
        }

        int n = 0;
        if ((err = records.num_records(n)) != 0)
        {
            return err;
        }

        if (n > 0 || (partial && m_records_set.empty()))
        {
            m_records_set.push_back(records);
        }

        bool overflow = false;
        if ((err = records.is_overflow(overflow)) != 0)
        {
            return err;
        }

        if (partial || overflow)
        {
            break;
        }
    }

    return 0;
}

int FetchResponseBlock::encode(packetEncoder &pe, int16_t version)
{
    pe.putKError(m_err);
    pe.putInt64(m_high_water_mark_offset);

    if (version >= 4)
    {
        pe.putInt64(m_last_stable_offset);
        if (version >= 5)
        {
            pe.putInt64(m_log_start_offset);
        }

        int err = pe.putArrayLength(static_cast<int32_t>(m_aborted_transactions.size()));
        if (err != 0)
        {
            return err;
        }

        for (auto &transact : m_aborted_transactions)
        {
            if ((err = transact.encode(pe)) != 0)
            {
                return err;
            }
        }
    }

    if (version >= 11)
    {
        pe.putInt32(m_preferred_read_replica);
    }
    LengthField length_field;
    pe.push(length_field);
    for (auto &records : m_records_set)
    {
        int err = records.encode(pe);
        if (err != 0)
        {
            pe.pop();
            return err;
        }
    }
    return pe.pop();
}

int FetchResponseBlock::num_records() const
{
    int sum = 0;
    for (auto &records : m_records_set)
    {
        int count = 0;
        int err = records.num_records(count);
        if (err != 0)
        {
            return err;
        }
        sum += count;
    }
    return sum;
}

int FetchResponseBlock::is_partial(bool &partial) const
{
    if (m_partial)
    {
        partial = true;
        return 0;
    }
    if (m_records_set.size() == 1)
    {
        m_records_set[0].is_partial(partial);
        return 0;
    }
    return INVALID;
}

std::vector<AbortedTransaction> &FetchResponseBlock::get_aborted_transactions()
{
    std::sort(m_aborted_transactions.begin(), m_aborted_transactions.end(),
              [](const auto &a, const auto &b)
              {
                  return a.m_first_offset < b.m_first_offset;
              });
    return m_aborted_transactions;
}

FetchResponse::FetchResponse()
    : m_version(0), m_error_code(0), m_session_id(0), m_log_append_time(false)
{
}

void FetchResponse::set_version(int16_t v)
{
    m_version = v;
}

int FetchResponse::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;
    int err = 0;

    if (m_version >= 1)
    {
        if ((err = pd.getDurationMs(m_throttle_time)) != 0)
        {
            return err;
        }
    }

    if (m_version >= 7)
    {
        if ((err = pd.getInt16(m_error_code)) != 0)
        {
            return err;
        }
        if ((err = pd.getInt32(m_session_id)) != 0)
        {
            return err;
        }
    }

    int32_t num_topics;
    if ((err = pd.getArrayLength(num_topics)) != 0)
    {
        return err;
    }

    m_blocks.clear();
    m_blocks.reserve(num_topics);
    for (int i = 0; i < num_topics; ++i)
    {
        std::string name;
        if ((err = pd.getString(name)) != 0)
        {
            return err;
        }

        int32_t num_blocks;
        if ((err = pd.getArrayLength(num_blocks)) != 0)
        {
            return err;
        }

        auto &partitions = m_blocks[name];
        for (int j = 0; j < num_blocks; ++j)
        {
            int32_t id;
            if ((err = pd.getInt32(id)) != 0)
            {
                return err;
            }

            if ((err = partitions[id].decode(pd, version)) != 0)
            {
                return err;
            }
        }
    }

    return 0;
}

int FetchResponse::encode(packetEncoder &pe)
{
    if (m_version >= 1)
    {
        pe.putDurationMs(m_throttle_time);
    }

    if (m_version >= 7)
    {
        pe.putInt16(m_error_code);
        pe.putInt32(m_session_id);
    }

    int err = pe.putArrayLength(static_cast<int32_t>(m_blocks.size()));
    if (err != 0)
        return err;

    for (auto &topic_pair : m_blocks)
    {
        const std::string &topic = topic_pair.first;
        auto &partitions = topic_pair.second;

        if ((err = pe.putString(topic)) != 0)
            return err;
        if ((err = pe.putArrayLength(static_cast<int32_t>(partitions.size()))) != 0)
            return err;

        for (auto &part_pair : partitions)
        {
            pe.putInt32(part_pair.first);
            if ((err = part_pair.second.encode(pe, m_version)) != 0)
                return err;
        }
    }

    return 0;
}

int16_t FetchResponse::key() const
{
    return apiKeyFetch;
}

int16_t FetchResponse::version() const
{
    return m_version;
}

int16_t FetchResponse::header_version() const
{
    return 0;
}

bool FetchResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 15;
}

KafkaVersion FetchResponse::required_version() const
{
    switch (m_version)
    {
    case 11:
        return V2_3_0_0;
    case 9:
    case 10:
        return V2_1_0_0;
    case 8:
        return V2_0_0_0;
    case 7:
        return V1_1_0_0;
    case 6:
        return V1_0_0_0;
    case 4:
    case 5:
        return V0_11_0_0;
    case 3:
        return V0_10_1_0;
    case 2:
        return V0_10_0_0;
    case 1:
        return V0_9_0_0;
    case 0:
        return V0_8_2_0;
    default:
        return V2_3_0_0;
    }
}

std::chrono::milliseconds FetchResponse::throttle_time() const
{
    return m_throttle_time;
}

const FetchResponseBlock &FetchResponse::get_block(const std::string &topic, int32_t partition) const
{
    auto tit = m_blocks.find(topic);
    if (tit != m_blocks.end())
    {
        auto pit = tit->second.find(partition);
        if (pit != tit->second.end())
        {
            return pit->second;
        }
    }
    static FetchResponseBlock _;
    return _;
}

FetchResponseBlock &FetchResponse::get_block(const std::string &topic, int32_t partition)
{
    auto tit = m_blocks.find(topic);
    if (tit != m_blocks.end())
    {
        auto pit = tit->second.find(partition);
        if (pit != tit->second.end())
        {
            return pit->second;
        }
    }
    static FetchResponseBlock _;
    return _;
}

void FetchResponse::add_error(const std::string &topic, int32_t partition, KError err)
{
    m_blocks[topic][partition].m_err = err;
}
FetchResponseBlock &FetchResponse::get_or_create_block(const std::string &topic, int32_t partition)
{
    auto tit = m_blocks.find(topic);
    if (tit != m_blocks.end())
    {
        auto pit = tit->second.find(partition);
        if (pit != tit->second.end())
        {
            return pit->second;
        }
    }
    static FetchResponseBlock _;
    return _;
}

static std::pair<std::string, std::string> encodeKV(Encoder *key, Encoder *value)
{
    std::string kb, vb;
    if (key)
    {
        key->Encode(kb);
    }
    if (value)
    {
        value->Encode(vb);
    }
    return {kb, vb};
}

void FetchResponse::add_message_with_timestamp(const std::string &topic, int32_t partition, Encoder *key, Encoder *value,
                                               int64_t offset, std::chrono::system_clock::time_point timestamp, int8_t version)
{
    auto &frb = m_blocks[topic][partition];
    auto kv = encodeKV(key, value);
    auto msgTimestamp = m_log_append_time ? m_timestamp : timestamp;
    auto msg = std::make_shared<Message>(kv.first, kv.second, m_log_append_time, msgTimestamp, version);
    auto msg_block = std::make_shared<MessageBlock>(msg, offset);

    if (frb.m_records_set.empty())
    {
        frb.m_records_set.emplace_back(std::make_shared<MessageSet>(msg_block));
    }
    frb.m_records_set[0].m_message_set->m_messages.emplace_back(msg_block);
}

void FetchResponse::add_record_with_timestamp(const std::string &topic, int32_t partition, Encoder *key, Encoder *value,
                                              int64_t offset, std::chrono::system_clock::time_point timestamp)
{
    auto &frb = m_blocks[topic][partition];
    auto kv = encodeKV(key, value);

    if (frb.m_records_set.empty())
    {
        frb.m_records_set.emplace_back(std::make_shared<RecordBatch>(2, m_log_append_time, timestamp, m_timestamp));
    }

    auto batch = frb.m_records_set[0].m_record_batch;
    auto rec = std::make_shared<Record>(kv.first, kv.second, offset, std::chrono::duration_cast<std::chrono::milliseconds>(timestamp - batch->m_first_timestamp));
    batch->add_record(rec);
}

void FetchResponse::AddRecordBatchWithTimestamp(const std::string &topic, int32_t partition, Encoder *key, Encoder *value,
                                                int64_t offset, int64_t producerID, bool isTransactional,
                                                std::chrono::system_clock::time_point timestamp)
{
    auto &frb = m_blocks[topic][partition];
    auto kv = encodeKV(key, value);

    auto batch = std::make_shared<RecordBatch>();
    batch->m_version = 2;
    batch->m_log_append_time = m_log_append_time;
    batch->m_first_timestamp = timestamp;
    batch->m_max_timestamp = m_timestamp;
    batch->m_first_offset = offset;
    batch->m_last_offset_delta = 0;
    batch->m_producer_id = producerID;
    batch->m_is_transactional = isTransactional;

    auto rec = std::make_shared<Record>(kv.first, kv.second, 0, std::chrono::duration_cast<std::chrono::milliseconds>(timestamp - batch->m_first_timestamp));
    batch->add_record(rec);
    frb.m_records_set.emplace_back(batch);
}

void FetchResponse::AddControlRecordWithTimestamp(const std::string &topic, int32_t partition, int64_t offset,
                                                  int64_t producerID, ControlRecordType recordType, std::chrono::system_clock::time_point timestamp)
{
    auto &frb = m_blocks[topic][partition];

    auto batch = std::make_shared<RecordBatch>();
    batch->m_version = 2;
    batch->m_log_append_time = m_log_append_time;
    batch->m_first_timestamp = timestamp;
    batch->m_max_timestamp = m_timestamp;
    batch->m_first_offset = offset;
    batch->m_last_offset_delta = 0;
    batch->m_producer_id = producerID;
    batch->m_is_transactional = true;
    batch->m_control = true;

    auto crAbort = std::make_shared<ControlRecord>(0, producerID, recordType);

    realEncoder crKey;
    realEncoder crValue;
    crAbort->encode(crKey, crValue);

    auto rec = std::make_shared<Record>(crKey.m_raw, crValue.m_raw, 0, std::chrono::duration_cast<std::chrono::milliseconds>(timestamp - batch->m_first_timestamp));
    batch->add_record(rec);
    frb.m_records_set.emplace_back(batch);
}

void FetchResponse::add_message(const std::string &topic, int32_t partition, Encoder *key, Encoder *value, int64_t offset)
{
    add_message_with_timestamp(topic, partition, key, value, offset, std::chrono::system_clock::time_point{}, 0);
}

void FetchResponse::add_record(const std::string &topic, int32_t partition, Encoder *key, Encoder *value, int64_t offset)
{
    add_record_with_timestamp(topic, partition, key, value, offset, std::chrono::system_clock::time_point{});
}

void FetchResponse::add_record_batch(const std::string &topic, int32_t partition, Encoder *key, Encoder *value, int64_t offset, int64_t producerID, bool isTransactional)
{
    AddRecordBatchWithTimestamp(topic, partition, key, value, offset, producerID, isTransactional, std::chrono::system_clock::time_point{});
}

void FetchResponse::add_control_record(const std::string &topic, int32_t partition, int64_t offset, int64_t producerID, ControlRecordType recordType)
{
    AddControlRecordWithTimestamp(topic, partition, offset, producerID, recordType, std::chrono::system_clock::time_point{});
}

void FetchResponse::set_last_offset_delta(const std::string &topic, int32_t partition, int32_t offset)
{
    auto &frb = m_blocks[topic][partition];
    if (frb.m_records_set.empty())
    {
        frb.m_records_set.emplace_back(std::make_shared<RecordBatch>(2));
    }
    frb.m_records_set[0].m_record_batch->m_last_offset_delta = offset;
}

void FetchResponse::set_last_stable_offset(const std::string &topic, int32_t partition, int64_t offset)
{
    auto &frb = m_blocks[topic][partition];
    frb.m_last_stable_offset = offset;
}