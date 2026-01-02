#include <algorithm>
#include "version.h"
#include "fetch_response.h"
#include "length_field.h"
#include "real_encoder.h"
AbortedTransaction::AbortedTransaction()
    : ProducerID(0), FirstOffset(0)
{
}

int AbortedTransaction::decode(PDecoder &pd)
{
    int err = 0;
    if ((err = pd.getInt64(ProducerID)) != 0)
        return err;
    if ((err = pd.getInt64(FirstOffset)) != 0)
        return err;
    return 0;
}

int AbortedTransaction::encode(PEncoder &pe)
{
    pe.putInt64(ProducerID);
    pe.putInt64(FirstOffset);
    return 0;
}

FetchResponseBlock::FetchResponseBlock()
    : Err(static_cast<KError>(0)), HighWaterMarkOffset(0), LastStableOffset(0), LogStartOffset(0),
      PreferredReadReplica(-1), Partial(false)
{
}

int FetchResponseBlock::decode(PDecoder &pd, int16_t version)
{
    int err = 0;

    if ((err = pd.getKError(Err)) != 0)
        return err;
    if ((err = pd.getInt64(HighWaterMarkOffset)) != 0)
        return err;

    if (version >= 4)
    {
        if ((err = pd.getInt64(LastStableOffset)) != 0)
            return err;
        if (version >= 5)
        {
            if ((err = pd.getInt64(LogStartOffset)) != 0)
                return err;
        }

        int32_t numTransact;
        if ((err = pd.getArrayLength(numTransact)) != 0)
            return err;

        if (numTransact >= 0)
        {
            AbortedTransactions.resize(numTransact);
            for (int i = 0; i < numTransact; ++i)
            {
                auto transact = std::make_shared<AbortedTransaction>();
                if ((err = transact->decode(pd)) != 0)
                    return err;
                AbortedTransactions[i] = transact;
            }
        }
    }

    if (version >= 11)
    {
        if ((err = pd.getInt32(PreferredReadReplica)) != 0)
            return err;
    }
    else
    {
        PreferredReadReplica = -1;
    }

    int32_t recordsSize;
    if ((err = pd.getInt32(recordsSize)) != 0)
    {
        return err;
    }

    std::shared_ptr<PDecoder> recordsDecoder_;
    if ((err = pd.getSubset(recordsSize, recordsDecoder_)) != 0)
    {
        return err;
    }

    RecordsSet.clear();
    while (recordsDecoder_->remaining() > 0)
    {
        auto records = std::make_shared<Records>();
        int decodeErr = records->decode(*recordsDecoder_);
        bool isInsufficientData = (decodeErr == ErrInsufficientData);

        if (decodeErr != 0 && !isInsufficientData)
        {
            return decodeErr;
        }

        if (isInsufficientData && RecordsSet.empty())
        {
            Partial = true;
            break;
        }

        int64_t nextOffset;
        if ((err = records->nextOffset(nextOffset)) != 0)
        {
            return err;
        }
        recordsNextOffset = nextOffset;

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

        if (n > 0 || (partial && RecordsSet.empty()))
        {
            RecordsSet.push_back(records);
            if (!Records_)
            {
                Records_ = records;
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

int FetchResponseBlock::encode(PEncoder &pe, int16_t version)
{
    pe.putKError(Err);
    pe.putInt64(HighWaterMarkOffset);

    if (version >= 4)
    {
        pe.putInt64(LastStableOffset);
        if (version >= 5)
        {
            pe.putInt64(LogStartOffset);
        }

        int err = pe.putArrayLength(static_cast<int32_t>(AbortedTransactions.size()));
        if (err != 0)
            return err;

        for (auto &transact : AbortedTransactions)
        {
            if ((err = transact->encode(pe)) != 0)
                return err;
        }
    }

    if (version >= 11)
    {
        pe.putInt32(PreferredReadReplica);
    }

    pe.push(std::dynamic_pointer_cast<pushEncoder>(std::make_shared<LengthField>()));
    for (auto &records : RecordsSet)
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
    for (auto &records : RecordsSet)
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
    if (Partial)
    {
        partial = true;
        return 0;
    }
    if (RecordsSet.size() == 1)
    {
        RecordsSet[0]->isPartial(partial);
        return true;
    }
    return false;
}

std::vector<std::shared_ptr<AbortedTransaction>> FetchResponseBlock::getAbortedTransactions()
{
    std::sort(AbortedTransactions.begin(), AbortedTransactions.end(),
              [](const std::shared_ptr<AbortedTransaction> &a, const std::shared_ptr<AbortedTransaction> &b)
              {
                  return a->FirstOffset < b->FirstOffset;
              });
    return AbortedTransactions;
}

FetchResponse::FetchResponse()
    : Version(0), ErrorCode(0), SessionID(0), LogAppendTime(false)
{
}

void FetchResponse::setVersion(int16_t v)
{
    Version = v;
}

int FetchResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    int err = 0;

    if (Version >= 1)
    {
        if ((err = pd.getDurationMs(ThrottleTime)) != 0)
            return err;
    }

    if (Version >= 7)
    {
        if ((err = pd.getInt16(ErrorCode)) != 0)
            return err;
        if ((err = pd.getInt32(SessionID)) != 0)
            return err;
    }

    int32_t numTopics;
    if ((err = pd.getArrayLength(numTopics)) != 0)
        return err;

    Blocks.clear();
    Blocks.reserve(numTopics);
    for (int i = 0; i < numTopics; ++i)
    {
        std::string name;
        if ((err = pd.getString(name)) != 0)
            return err;

        int32_t numBlocks;
        if ((err = pd.getArrayLength(numBlocks)) != 0)
            return err;

        auto &partitions = Blocks[name];
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

int FetchResponse::encode(PEncoder &pe)
{
    if (Version >= 1)
    {
        pe.putDurationMs(ThrottleTime);
    }

    if (Version >= 7)
    {
        pe.putInt16(ErrorCode);
        pe.putInt32(SessionID);
    }

    int err = pe.putArrayLength(static_cast<int32_t>(Blocks.size()));
    if (err != 0)
        return err;

    for (auto &topic_pair : Blocks)
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
            if ((err = part_pair.second->encode(pe, Version)) != 0)
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
    return Version;
}

int16_t FetchResponse::headerVersion() const
{
    return 0;
}

bool FetchResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 11;
}

KafkaVersion FetchResponse::requiredVersion() const
{
    switch (Version)
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

std::chrono::milliseconds FetchResponse::throttleTime() const
{
    return ThrottleTime;
}

std::shared_ptr<FetchResponseBlock> FetchResponse::GetBlock(const std::string &topic, int32_t partition)
{
    auto topicIt = Blocks.find(topic);
    if (topicIt == Blocks.end())
        return nullptr;
    auto partIt = topicIt->second.find(partition);
    if (partIt == topicIt->second.end())
        return nullptr;
    return partIt->second;
}

void FetchResponse::AddError(const std::string &topic, int32_t partition, KError err)
{
    auto frb = getOrCreateBlock(topic, partition);
    frb->Err = err;
}

std::shared_ptr<FetchResponseBlock> FetchResponse::getOrCreateBlock(const std::string &topic, int32_t partition)
{
    auto &partitions = Blocks[topic];
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

    if (frb->RecordsSet.empty())
    {
        auto records = Records::NewLegacyRecords(std::make_shared<MessageSet>(msgBlock));
        frb->RecordsSet.emplace_back(records);
    }
    auto set = frb->RecordsSet[0]->MsgSet;
    set->Messages.emplace_back(msgBlock);
}

void FetchResponse::AddRecordWithTimestamp(const std::string &topic, int32_t partition, Encoder *key, Encoder *value,
                                           int64_t offset, std::chrono::system_clock::time_point timestamp)
{
    auto frb = getOrCreateBlock(topic, partition);
    auto kv = encodeKV(key, value);

    if (frb->RecordsSet.empty())
    {
        auto batch = std::make_shared<RecordBatch>(2, LogAppendTime, timestamp, Timestamp);
        auto records = Records::NewDefaultRecords(batch);
        frb->RecordsSet.push_back(records);
    }

    auto batch = frb->RecordsSet[0]->RBatch;
    auto rec = std::make_shared<Record>(kv.first, kv.second, offset, std::chrono::duration_cast<std::chrono::milliseconds>(timestamp - batch->FirstTimestamp));
    batch->AddRecord(rec);
}

void FetchResponse::AddRecordBatchWithTimestamp(const std::string &topic, int32_t partition, Encoder *key, Encoder *value,
                                                int64_t offset, int64_t producerID, bool isTransactional,
                                                std::chrono::system_clock::time_point timestamp)
{
    auto frb = getOrCreateBlock(topic, partition);
    auto kv = encodeKV(key, value);

    auto batch = std::make_shared<RecordBatch>();
    batch->Version = 2;
    batch->LogAppendTime = LogAppendTime;
    batch->FirstTimestamp = timestamp;
    batch->MaxTimestamp = Timestamp;
    batch->FirstOffset = offset;
    batch->LastOffsetDelta = 0;
    batch->ProducerID = producerID;
    batch->IsTransactional = isTransactional;

    auto rec = std::make_shared<Record>(kv.first, kv.second, 0, std::chrono::duration_cast<std::chrono::milliseconds>(timestamp - batch->FirstTimestamp));
    batch->AddRecord(rec);

    auto records = std::make_shared<Records>();
    records->RBatch = batch;
    frb->RecordsSet.push_back(records);
}

void FetchResponse::AddControlRecordWithTimestamp(const std::string &topic, int32_t partition, int64_t offset,
                                                  int64_t producerID, ControlRecordType recordType,
                                                  std::chrono::system_clock::time_point timestamp)
{
    auto frb = getOrCreateBlock(topic, partition);

    auto batch = std::make_shared<RecordBatch>();
    batch->Version = 2;
    batch->LogAppendTime = LogAppendTime;
    batch->FirstTimestamp = timestamp;
    batch->MaxTimestamp = Timestamp;
    batch->FirstOffset = offset;
    batch->LastOffsetDelta = 0;
    batch->ProducerID = producerID;
    batch->IsTransactional = true;
    batch->Control = true;

    auto records = std::make_shared<Records>();
    records->RBatch = batch;

    auto crAbort = std::make_shared<ControlRecord>(0, producerID, recordType);

    realEncoder crKey;
    realEncoder crValue;
    crAbort->encode(crKey, crValue);

    auto rec = std::make_shared<Record>(crKey.Raw, crValue.Raw, 0, std::chrono::duration_cast<std::chrono::milliseconds>(timestamp - batch->FirstTimestamp));
    batch->AddRecord(rec);

    frb->RecordsSet.push_back(records);
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
    if (frb->RecordsSet.empty())
    {
        auto batch = std::make_shared<RecordBatch>(2);
        auto records = Records::NewDefaultRecords(batch);
        frb->RecordsSet.push_back(records);
    }
    frb->RecordsSet[0]->RBatch->LastOffsetDelta = offset;
}

void FetchResponse::SetLastStableOffset(const std::string &topic, int32_t partition, int64_t offset)
{
    auto frb = getOrCreateBlock(topic, partition);
    frb->LastStableOffset = offset;
}