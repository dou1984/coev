#include <algorithm>
#include "version.h"
#include "fetch_response.h"
#include "length_field.h"
#include "real_encoder.h"
AbortedTransaction::AbortedTransaction()
    : m_producer_id(0), m_first_offset(0)
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
        return err;
    if ((err = pd.getInt64(m_high_water_mark_offset)) != 0)
        return err;

    if (version >= 4)
    {
        if ((err = pd.getInt64(m_last_stable_offset)) != 0)
            return err;
        if (version >= 5)
        {
            if ((err = pd.getInt64(m_log_start_offset)) != 0)
                return err;
        }

        int32_t numTransact;
        if ((err = pd.getArrayLength(numTransact)) != 0)
            return err;

        if (numTransact >= 0)
        {
            m_aborted_transactions.resize(numTransact);
            for (int i = 0; i < numTransact; ++i)
            {
                auto transact = std::make_shared<AbortedTransaction>();
                if ((err = transact->decode(pd)) != 0)
                    return err;
                m_aborted_transactions[i] = transact;
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

    std::shared_ptr<packetDecoder> recordsDecoder_;
    if ((err = pd.getSubset(recordsSize, recordsDecoder_)) != 0)
    {
        return err;
    }

    m_records_set.clear();
    while (recordsDecoder_->remaining() > 0)
    {
        auto records = std::make_shared<Records>();
        int decodeErr = records->decode(*recordsDecoder_);
        bool isInsufficientData = (decodeErr == ErrInsufficientData);

        if (decodeErr != 0 && !isInsufficientData)
        {
            return decodeErr;
        }

        if (isInsufficientData && m_records_set.empty())
        {
            m_partial = true;
            break;
        }

        int64_t nextOffset;
        if ((err = records->nextOffset(nextOffset)) != 0)
        {
            return err;
        }
        m_records_next_offset = nextOffset;

        bool partial = false;
        if ((err = records->isPartial(partial)) != 0)
        {
            return err;
        }

        int n = 0;
        if ((err = records->numRecords(n)) != 0)
        {
            return err;
        }

        if (n > 0 || (partial && m_records_set.empty()))
        {
            m_records_set.push_back(records);
            if (!m_records)
            {
                m_records = records;
            }
        }

        bool overflow = false;
        if ((err = records->isOverflow(overflow)) != 0)
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
            return err;

        for (auto &transact : m_aborted_transactions)
        {
            if ((err = transact->encode(pe)) != 0)
                return err;
        }
    }

    if (version >= 11)
    {
        pe.putInt32(m_preferred_read_replica);
    }

    pe.push(std::dynamic_pointer_cast<pushEncoder>(std::make_shared<LengthField>()));
    for (auto &records : m_records_set)
    {
        int err = records->encode(pe);
        if (err != 0)
        {
            pe.pop();
            return err;
        }
    }
    return pe.pop();
}

int FetchResponseBlock::numRecords()
{
    int sum = 0;
    for (auto &records : m_records_set)
    {
        int count = 0;
        int err = records->numRecords(count);
        if (err != 0)
            return err;
        sum += count;
    }
    return sum;
}

int FetchResponseBlock::isPartial(bool &partial)
{
    if (m_partial)
    {
        partial = true;
        return 0;
    }
    if (m_records_set.size() == 1)
    {
        m_records_set[0]->isPartial(partial);
        return true;
    }
    return false;
}

std::vector<std::shared_ptr<AbortedTransaction>> FetchResponseBlock::getAbortedTransactions()
{
    std::sort(m_aborted_transactions.begin(), m_aborted_transactions.end(),
              [](const std::shared_ptr<AbortedTransaction> &a, const std::shared_ptr<AbortedTransaction> &b)
              {
                  return a->m_first_offset < b->m_first_offset;
              });
    return m_aborted_transactions;
}

FetchResponse::FetchResponse()
    : m_version(0), ErrorCode(0), SessionID(0), LogAppendTime(false)
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
            return err;
    }

    if (m_version >= 7)
    {
        if ((err = pd.getInt16(ErrorCode)) != 0)
            return err;
        if ((err = pd.getInt32(SessionID)) != 0)
            return err;
    }

    int32_t numTopics;
    if ((err = pd.getArrayLength(numTopics)) != 0)
        return err;

    m_blocks.clear();
    m_blocks.reserve(numTopics);
    for (int i = 0; i < numTopics; ++i)
    {
        std::string name;
        if ((err = pd.getString(name)) != 0)
            return err;

        int32_t numBlocks;
        if ((err = pd.getArrayLength(numBlocks)) != 0)
            return err;

        auto &partitions = m_blocks[name];
        partitions.reserve(numBlocks);
        for (int j = 0; j < numBlocks; ++j)
        {
            int32_t id;
            if ((err = pd.getInt32(id)) != 0)
                return err;

            auto block = std::make_shared<FetchResponseBlock>();
            if ((err = block->decode(pd, version)) != 0)
                return err;
            partitions[id] = block;
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
        pe.putInt16(ErrorCode);
        pe.putInt32(SessionID);
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
            if ((err = part_pair.second->encode(pe, m_version)) != 0)
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

KafkaVersion FetchResponse::required_version()  const
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

std::shared_ptr<FetchResponseBlock> FetchResponse::GetBlock(const std::string &topic, int32_t partition)
{
    auto topicIt = m_blocks.find(topic);
    if (topicIt == m_blocks.end())
        return nullptr;
    auto partIt = topicIt->second.find(partition);
    if (partIt == topicIt->second.end())
        return nullptr;
    return partIt->second;
}

void FetchResponse::AddError(const std::string &topic, int32_t partition, KError err)
{
    auto frb = getOrCreateBlock(topic, partition);
    frb->m_err = err;
}

std::shared_ptr<FetchResponseBlock> FetchResponse::getOrCreateBlock(const std::string &topic, int32_t partition)
{
    auto &partitions = m_blocks[topic];
    auto it = partitions.find(partition);
    if (it == partitions.end())
    {
        auto block = std::make_shared<FetchResponseBlock>();
        partitions[partition] = block;
        return block;
    }
    return it->second;
}

static std::pair<std::string, std::string> encodeKV(Encoder *key, Encoder *value)
{
    std::string kb, vb;
    if (key)
        key->Encode(kb);
    if (value)
        value->Encode(vb);
    return {kb, vb};
}

void FetchResponse::AddMessageWithTimestamp(const std::string &topic, int32_t partition, Encoder *key, Encoder *value,
                                            int64_t offset, std::chrono::system_clock::time_point timestamp, int8_t version)
{
    auto frb = getOrCreateBlock(topic, partition);
    auto kv = encodeKV(key, value);
    auto msgTimestamp = LogAppendTime ? Timestamp : timestamp;
    auto msg = std::make_shared<Message>(kv.first, kv.second, LogAppendTime, msgTimestamp, version);
    auto msgBlock = std::make_shared<MessageBlock>(msg, offset);

    if (frb->m_records_set.empty())
    {
        auto records = Records::NewLegacyRecords(std::make_shared<MessageSet>(msgBlock));
        frb->m_records_set.emplace_back(records);
    }
    auto set = frb->m_records_set[0]->m_msg_set;
    set->m_messages.emplace_back(msgBlock);
}

void FetchResponse::AddRecordWithTimestamp(const std::string &topic, int32_t partition, Encoder *key, Encoder *value,
                                           int64_t offset, std::chrono::system_clock::time_point timestamp)
{
    auto frb = getOrCreateBlock(topic, partition);
    auto kv = encodeKV(key, value);

    if (frb->m_records_set.empty())
    {
        auto batch = std::make_shared<RecordBatch>(2, LogAppendTime, timestamp, Timestamp);
        auto records = Records::NewDefaultRecords(batch);
        frb->m_records_set.push_back(records);
    }

    auto batch = frb->m_records_set[0]->m_record_batch;
    auto rec = std::make_shared<Record>(kv.first, kv.second, offset, std::chrono::duration_cast<std::chrono::milliseconds>(timestamp - batch->m_first_timestamp));
    batch->AddRecord(rec);
}

void FetchResponse::AddRecordBatchWithTimestamp(const std::string &topic, int32_t partition, Encoder *key, Encoder *value,
                                                int64_t offset, int64_t producerID, bool isTransactional,
                                                std::chrono::system_clock::time_point timestamp)
{
    auto frb = getOrCreateBlock(topic, partition);
    auto kv = encodeKV(key, value);

    auto batch = std::make_shared<RecordBatch>();
    batch->m_version = 2;
    batch->m_log_append_time = LogAppendTime;
    batch->m_first_timestamp = timestamp;
    batch->m_max_timestamp = Timestamp;
    batch->m_first_offset = offset;
    batch->m_last_offset_delta = 0;
    batch->m_producer_id = producerID;
    batch->m_is_transactional = isTransactional;

    auto rec = std::make_shared<Record>(kv.first, kv.second, 0, std::chrono::duration_cast<std::chrono::milliseconds>(timestamp - batch->m_first_timestamp));
    batch->AddRecord(rec);

    auto records = std::make_shared<Records>();
    records->m_record_batch = batch;
    frb->m_records_set.push_back(records);
}

void FetchResponse::AddControlRecordWithTimestamp(const std::string &topic, int32_t partition, int64_t offset,
                                                  int64_t producerID, ControlRecordType recordType,
                                                  std::chrono::system_clock::time_point timestamp)
{
    auto frb = getOrCreateBlock(topic, partition);

    auto batch = std::make_shared<RecordBatch>();
    batch->m_version = 2;
    batch->m_log_append_time = LogAppendTime;
    batch->m_first_timestamp = timestamp;
    batch->m_max_timestamp = Timestamp;
    batch->m_first_offset = offset;
    batch->m_last_offset_delta = 0;
    batch->m_producer_id = producerID;
    batch->m_is_transactional = true;
    batch->m_control = true;

    auto records = std::make_shared<Records>();
    records->m_record_batch = batch;

    auto crAbort = std::make_shared<ControlRecord>(0, producerID, recordType);

    realEncoder crKey;
    realEncoder crValue;
    crAbort->encode(crKey, crValue);

    auto rec = std::make_shared<Record>(crKey.m_raw, crValue.m_raw, 0, std::chrono::duration_cast<std::chrono::milliseconds>(timestamp - batch->m_first_timestamp));
    batch->AddRecord(rec);

    frb->m_records_set.push_back(records);
}

void FetchResponse::AddMessage(const std::string &topic, int32_t partition, Encoder *key, Encoder *value, int64_t offset)
{
    AddMessageWithTimestamp(topic, partition, key, value, offset, std::chrono::system_clock::time_point{}, 0);
}

void FetchResponse::AddRecord(const std::string &topic, int32_t partition, Encoder *key, Encoder *value, int64_t offset)
{
    AddRecordWithTimestamp(topic, partition, key, value, offset, std::chrono::system_clock::time_point{});
}

void FetchResponse::AddRecordBatch(const std::string &topic, int32_t partition, Encoder *key, Encoder *value,
                                   int64_t offset, int64_t producerID, bool isTransactional)
{
    AddRecordBatchWithTimestamp(topic, partition, key, value, offset, producerID, isTransactional,
                                std::chrono::system_clock::time_point{});
}

void FetchResponse::AddControlRecord(const std::string &topic, int32_t partition, int64_t offset,
                                     int64_t producerID, ControlRecordType recordType)
{
    AddControlRecordWithTimestamp(topic, partition, offset, producerID, recordType,
                                  std::chrono::system_clock::time_point{});
}

void FetchResponse::SetLastOffsetDelta(const std::string &topic, int32_t partition, int32_t offset)
{
    auto frb = getOrCreateBlock(topic, partition);
    if (frb->m_records_set.empty())
    {
        auto batch = std::make_shared<RecordBatch>(2);
        auto records = Records::NewDefaultRecords(batch);
        frb->m_records_set.push_back(records);
    }
    frb->m_records_set[0]->m_record_batch->m_last_offset_delta = offset;
}

void FetchResponse::SetLastStableOffset(const std::string &topic, int32_t partition, int64_t offset)
{
    auto frb = getOrCreateBlock(topic, partition);
    frb->m_last_stable_offset = offset;
}