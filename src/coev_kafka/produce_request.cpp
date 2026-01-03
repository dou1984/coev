#include <cmath>
#include <limits>
#include "version.h"
#include "produce_request.h"
#include "length_field.h"
#include "metrics.h"

void ProduceRequest::setVersion(int16_t v)
{
    Version = v;
}

int ProduceRequest::encode(PEncoder &pe)
{
    if (Version >= 3)
    {
        if (int err = pe.putNullableString(TransactionalID); err != 0)
        {
            return err;
        }
    }

    pe.putInt16(static_cast<int16_t>(Acks));
    pe.putDurationMs(Timeout);

    auto metricRegistry = pe.metricRegistry();
    std::shared_ptr<metrics::Histogram> batchSizeMetric;
    std::shared_ptr<metrics::Histogram> compressionRatioMetric;
    if (metricRegistry)
    {
        batchSizeMetric = metrics::GetOrRegisterHistogram("batch-size", metricRegistry);
        compressionRatioMetric = metrics::GetOrRegisterHistogram("compression-ratio", metricRegistry);
    }

    int64_t totalRecordCount = 0;

    if (int err = pe.putArrayLength(static_cast<int32_t>(records.size())); err != 0)
    {
        return err;
    }

    for (auto &[topic, partitions] : records)
    {
        if (int err = pe.putString(topic); err != 0)
            return err;
        if (int err = pe.putArrayLength(static_cast<int32_t>(partitions.size())); err != 0)
            return err;

        int64_t topicRecordCount = 0;
        std::shared_ptr<metrics::Histogram> topicCompressionRatioMetric;

        topicCompressionRatioMetric = metrics::GetOrRegisterTopicHistogram("compression-ratio", topic, metricRegistry);

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

            if (metricRegistry)
            {
                if (Version >= 3)
                {
                    topicRecordCount += UpdateBatchMetrics(recs->RBatch, compressionRatioMetric, topicCompressionRatioMetric);
                }
                else
                {
                    topicRecordCount += UpdateMsgSetMetrics(recs->MsgSet, compressionRatioMetric, topicCompressionRatioMetric);
                }

                int64_t batchSize = static_cast<int64_t>(pe.offset() - startOffset);
                batchSizeMetric->Update(batchSize);
                metrics::GetOrRegisterTopicHistogram("batch-size", topic, metricRegistry)->Update(batchSize);
            }
        }

        if (topicRecordCount > 0 && metricRegistry)
        {
            metrics::GetOrRegisterTopicMeter("record-send-rate", topic, metricRegistry)->Mark(topicRecordCount);
            metrics::GetOrRegisterTopicHistogram("records-per-request", topic, metricRegistry)->Update(topicRecordCount);
            totalRecordCount += topicRecordCount;
        }
    }

    if (totalRecordCount > 0 && metricRegistry)
    {
        metrics::GetOrRegisterMeter("record-send-rate", metricRegistry)->Mark(totalRecordCount);
        metrics::GetOrRegisterHistogram("records-per-request", metricRegistry)->Update(totalRecordCount);
    }

    return 0;
}

int ProduceRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    if (version >= 3)
    {
        if (int err = pd.getNullableString(TransactionalID); err != 0)
            return err;
    }

    int16_t acks;
    if (int err = pd.getInt16(acks); err != 0)
        return err;
    Acks = static_cast<RequiredAcks>(acks);

    if (int err = pd.getDurationMs(Timeout); err != 0)
        return err;

    int32_t topicCount;
    if (int err = pd.getArrayLength(topicCount); err != 0)
        return err;
    if (topicCount == 0)
        return 0;

    records.clear();
    for (int i = 0; i < topicCount; ++i)
    {
        std::string topic;
        if (int err = pd.getString(topic); err != 0)
            return err;

        int32_t partitionCount;
        if (int err = pd.getArrayLength(partitionCount); err != 0)
            return err;

        auto &partMap = records[topic];
        for (int j = 0; j < partitionCount; ++j)
        {
            int32_t partition;
            if (int err = pd.getInt32(partition); err != 0)
                return err;

            int32_t size;
            if (int err = pd.getInt32(size); err != 0)
                return err;

            std::shared_ptr<PDecoder> subset;
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
    return Version;
}

int16_t ProduceRequest::headerVersion() const
{
    return 1;
}

bool ProduceRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 7;
}

KafkaVersion ProduceRequest::requiredVersion() const
{
    switch (Version)
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

    if (records[topic][partition] == nullptr)
    {
        auto msgSet = std::make_shared<MessageSet>();
        msgSet->addMessage(msg);
        records[topic][partition] = Records::NewLegacyRecords(msgSet);
    }
    else
    {
        records[topic][partition]->MsgSet->addMessage(msg);
    }
}

void ProduceRequest::AddSet(const std::string &topic, int32_t partition, std::shared_ptr<MessageSet> set)
{
    EnsureRecords(topic, partition);
    records[topic][partition] = Records::NewLegacyRecords(set);
}

void ProduceRequest::AddBatch(const std::string &topic, int32_t partition, std::shared_ptr<RecordBatch> batch)
{
    EnsureRecords(topic, partition);
    records[topic][partition] = Records::NewDefaultRecords(batch);
}

int64_t ProduceRequest::UpdateMsgSetMetrics(const std::shared_ptr<MessageSet> msgSet, std::shared_ptr<metrics::Histogram> compressionRatioMetric, std::shared_ptr<metrics::Histogram> topicCompressionRatioMetric)
{
    if (!msgSet)
    {
        return 0;
    }
    int64_t count = 0;
    for (auto &block : msgSet->Messages)
    {
        if (block->Msg->Set)
        {
            count += static_cast<int64_t>(block->Msg->Set->Messages.size());
        }
        else
        {
            ++count;
        }

        if (block->Msg->compressedSize != 0 && compressionRatioMetric)
        {
            double ratio = static_cast<double>(block->Msg->Value.size()) / block->Msg->compressedSize;
            int64_t intRatio = static_cast<int64_t>(100 * ratio);
            compressionRatioMetric->Update(intRatio);
            if (topicCompressionRatioMetric)
            {
                topicCompressionRatioMetric->Update(intRatio);
            }
        }
    }
    return count;
}

int64_t ProduceRequest::UpdateBatchMetrics(const std::shared_ptr<RecordBatch> batch, std::shared_ptr<metrics::Histogram> compressionRatioMetric, std::shared_ptr<metrics::Histogram> topicCompressionRatioMetric)
{
    if (batch == nullptr || batch->compressedRecords.empty())
    {
        return static_cast<int64_t>(batch ? batch->Records.size() : 0);
    }

    if (compressionRatioMetric)
    {
        double ratio = static_cast<double>(batch->recordsLen) / batch->compressedRecords.size();
        int64_t intRatio = static_cast<int64_t>(100 * ratio);
        compressionRatioMetric->Update(intRatio);
        if (topicCompressionRatioMetric)
        {
            topicCompressionRatioMetric->Update(intRatio);
        }
    }

    return static_cast<int64_t>(batch->Records.size());
}