#include "version.h"
#include "produce_set.h"
#include "async_producer.h"
#include "producer_message.h"
#include "records.h"
#include "record_batch.h"
#include "message_set.h"
#include "produce_request.h"
#include "logger.h"
#include <chrono>
#include <algorithm>
#include <stdexcept>
#include "metadata_request.h"
#include "metadata_response.h"
#include "consumer_metadata_request.h"
#include "consumer_metadata_response.h"
#include "find_coordinator_request.h"
#include "find_coordinator_response.h"
#include "offset_request.h"
#include "offset_response.h"
#include "produce_request.h"
#include "produce_response.h"
#include "fetch_request.h"
#include "fetch_response.h"
#include "offset_commit_request.h"
#include "offset_commit_response.h"
#include "offset_fetch_request.h"
#include "offset_fetch_response.h"
#include "join_group_request.h"
#include "join_group_response.h"
#include "sync_group_request.h"
#include "sync_group_response.h"
#include "leave_group_request.h"
#include "leave_group_response.h"
#include "heartbeat_request.h"
#include "heartbeat_response.h"
#include "list_groups_request.h"
#include "list_groups_response.h"
#include "describe_groups_request.h"
#include "describe_groups_response.h"
#include "api_versions_request.h"
#include "api_versions_response.h"
#include "create_topics_request.h"
#include "create_topics_response.h"
#include "delete_topics_request.h"
#include "delete_topics_response.h"
#include "create_partitions_request.h"
#include "create_partitions_response.h"
#include "alter_partition_reassignments_request.h"
#include "alter_partition_reassignments_response.h"
#include "list_partition_reassignments_request.h"
#include "list_partition_reassignments_response.h"
#include "elect_leaders_request.h"
#include "elect_leaders_response.h"
#include "delete_records_request.h"
#include "delete_records_response.h"
#include "acl_describe_request.h"
#include "acl_describe_response.h"
#include "acl_create_request.h"
#include "acl_create_response.h"
#include "acl_delete_request.h"
#include "acl_delete_response.h"
#include "init_producer_id_request.h"
#include "init_producer_id_response.h"
#include "add_partitions_to_txn_request.h"
#include "add_partitions_to_txn_response.h"
#include "add_offsets_to_txn_request.h"
#include "add_offsets_to_txn_response.h"
#include "end_txn_request.h"
#include "end_txn_response.h"
#include "txn_offset_commit_request.h"
#include "txn_offset_commit_response.h"
#include "describe_configs_request.h"
#include "describe_configs_response.h"
#include "alter_configs_request.h"
#include "alter_configs_response.h"
#include "incremental_alter_configs_request.h"
#include "incremental_alter_configs_response.h"
#include "delete_groups_request.h"
#include "delete_groups_response.h"
#include "delete_offsets_request.h"
#include "delete_offsets_response.h"
#include "describe_log_dirs_request.h"
#include "describe_log_dirs_response.h"
#include "describe_user_scram_credentials_request.h"
#include "describe_user_scram_credentials_response.h"
#include "alter_user_scram_credentials_request.h"
#include "alter_user_scram_credentials_response.h"
#include "describe_client_quotas_request.h"
#include "describe_client_quotas_response.h"
#include "alter_client_quotas_request.h"
#include "alter_client_quotas_response.h"
#include "sasl_handshake_request.h"
#include "sasl_handshake_response.h"
#include "sasl_authenticate_response.h"
#include "sasl_authenticate_request.h"
#include "compress.h"

ProduceSet::ProduceSet(std::shared_ptr<AsyncProducer> parent)
    : Parent(parent)
{
    auto [pid, epoch] = parent->txnmgr_->GetProducerID();
    producerID = pid;
    producerEpoch = epoch;
}

int ProduceSet::Add(std::shared_ptr<ProducerMessage> msg)
{
    std::string key, val;
    if (msg->Key != nullptr)
    {
        if (int err = msg->Key->Encode(key); err != 0)
            return err;
    }
    if (msg->Value != nullptr)
    {
        if (int err = msg->Value->Encode(val); err != 0)
            return err;
    }

    auto timestamp = msg->Timestamp;
    if (timestamp == std::chrono::system_clock::time_point{})
    {
        timestamp = std::chrono::system_clock::now();
    }
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count();
    timestamp = std::chrono::system_clock::from_time_t(ms / 1000) + std::chrono::milliseconds(ms % 1000);

    auto &partitions = msgs[msg->Topic];

    int size = 0;
    auto setIt = partitions.find(msg->Partition);
    std::shared_ptr<PartitionSet> set;
    bool isNewSet = false;
    if (setIt == partitions.end())
    {
        if (Parent->conf_->Version.IsAtLeast(V0_11_0_0))
        {
            auto batch = std::make_shared<RecordBatch>();
            batch->FirstTimestamp = timestamp;
            batch->Version = 2;
            batch->Codec = Parent->conf_->Producer.Compression;
            batch->CompressionLevel = Parent->conf_->Producer.CompressionLevel;
            batch->ProducerID = producerID;
            batch->ProducerEpoch = producerEpoch;
            if (Parent->conf_->Producer.Idempotent)
            {
                batch->FirstSequence = msg->SequenceNumber;
            }
            set = std::make_shared<PartitionSet>();
            set->recordsToSend = Records::NewDefaultRecords(batch);
            size = recordBatchOverhead;
        }
        else
        {
            set = std::make_shared<PartitionSet>();
            set->recordsToSend = Records::NewLegacyRecords(std::make_shared<MessageSet>());
        }
        partitions[msg->Partition] = set;
        isNewSet = true;
    }
    else
    {
        set = setIt->second;
    }

    if (Parent->conf_->Version.IsAtLeast(V0_11_0_0))
    {
        if (Parent->conf_->Producer.Idempotent && msg->SequenceNumber < set->recordsToSend->RBatch->FirstSequence)
        {
            return -1; // assertion failed: message out of sequence added to a batch
        }
    }

    set->msgs.push_back(msg);

    if (Parent->conf_->Version.IsAtLeast(V0_11_0_0))
    {
        size += maximumRecordOverhead;
        auto rec = std::make_shared<Record>();
        rec->Key = key;
        rec->Value = val;
        rec->TimestampDelta = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp - set->recordsToSend->RBatch->FirstTimestamp);

        size += key.size() + val.size();
        if (!msg->Headers.empty())
        {
            rec->Headers.reserve(msg->Headers.size());
            for (auto &h : msg->Headers)
            {
                auto header = std::make_shared<RecordHeader>(h);
                rec->Headers.push_back(header);
                size += header->Key.size() + header->Value.size() + 2 * 5; // MaxVarintLen32 ~5
            }
        }
        set->recordsToSend->RBatch->AddRecord(rec);
    }
    else
    {
        auto msgToSend = std::make_shared<Message>();
        msgToSend->Codec = CompressionCodec::None;
        msgToSend->Key = key;
        msgToSend->Value = val;
        if (Parent->conf_->Version.IsAtLeast(V0_10_0_0))
        {
            msgToSend->Timestamp_.set_time(timestamp);
            msgToSend->Version = 1;
        }
        set->recordsToSend->MsgSet->addMessage(msgToSend);
        size = producerMessageOverhead + key.size() + val.size();
    }

    set->bufferBytes += size;
    bufferBytes += size;
    bufferCount++;

    return 0;
}

std::shared_ptr<ProduceRequest> ProduceSet::BuildRequest()
{
    auto req = std::make_shared<ProduceRequest>();
    req->Acks_ = Parent->conf_->Producer.Acks;
    req->Timeout = Parent->conf_->Producer.Timeout;

    if (Parent->conf_->Version.IsAtLeast(V0_10_0_0))
        req->Version = 2;
    if (Parent->conf_->Version.IsAtLeast(V0_11_0_0))
        req->Version = 3;
    if (Parent->conf_->Version.IsAtLeast(V1_0_0_0))
        req->Version = 5;
    if (Parent->conf_->Version.IsAtLeast(V2_0_0_0))
        req->Version = 6;
    if (Parent->conf_->Version.IsAtLeast(V2_1_0_0))
        req->Version = 7;

    if (Parent->conf_->Version.IsAtLeast(V0_11_0_0) && Parent->IsTransactional())
    {
        req->TransactionalID = Parent->conf_->Producer.Transaction.ID;
    }

    for (auto &it : msgs)
    {
        auto &topic = it.first;
        auto &partitionSets = it.second;
        for (auto &pit : partitionSets)
        {
            auto &partition = pit.first;
            auto &set = pit.second;
            if (req->Version >= 3)
            {
                auto rb = set->recordsToSend->RBatch;
                if (!rb->Records.empty())
                {
                    rb->LastOffsetDelta = static_cast<int32_t>(rb->Records.size() - 1);
                    std::chrono::milliseconds maxTimestampDelta(0);
                    for (size_t i = 0; i < rb->Records.size(); ++i)
                    {
                        rb->Records[i]->OffsetDelta = static_cast<int64_t>(i);
                        maxTimestampDelta = std::max(maxTimestampDelta,
                                                     std::chrono::duration_cast<std::chrono::milliseconds>(rb->Records[i]->TimestampDelta));
                    }
                    rb->MaxTimestamp = rb->FirstTimestamp + maxTimestampDelta;
                }
                rb->IsTransactional = Parent->IsTransactional();
                req->AddBatch(topic, partition, rb);
                continue;
            }

            if (Parent->conf_->Producer.Compression == CompressionCodec::None)
            {
                req->AddSet(topic, partition, set->recordsToSend->MsgSet);
            }
            else
            {
                if (Parent->conf_->Version.IsAtLeast(V0_10_0_0))
                {
                    for (size_t i = 0; i < set->recordsToSend->MsgSet->Messages.size(); ++i)
                    {
                        set->recordsToSend->MsgSet->Messages[i]->Offset = static_cast<int64_t>(i);
                    }
                }

                std::string payload;
                ::encode(set->recordsToSend->MsgSet, payload, Parent->MetricsRegistry);

                auto compMsg = std::make_shared<Message>();
                compMsg->Codec = Parent->conf_->Producer.Compression;
                compMsg->CompressionLevel = Parent->conf_->Producer.CompressionLevel;
                compMsg->Key.clear();
                compMsg->Value = payload;
                compMsg->Set = set->recordsToSend->MsgSet;

                if (Parent->conf_->Version.IsAtLeast(V0_10_0_0))
                {
                    compMsg->Version = 1;
                    if (!set->recordsToSend->MsgSet->Messages.empty())
                    {
                        compMsg->Timestamp_ = set->recordsToSend->MsgSet->Messages[0]->Msg->Timestamp_;
                    }
                }
                req->AddMessage(topic, partition, compMsg);
            }
        }
    }

    return req;
}

void ProduceSet::EachPartition(std::function<void(const std::string &, int32_t, std::shared_ptr<PartitionSet>)> cb)
{
    for (auto &it : msgs)
    {
        auto &topic = it.first;
        auto &partitionSet = it.second;
        for (auto &pit : partitionSet)
        {
            auto &partition = pit.first;
            auto &set = pit.second;
            cb(topic, partition, set);
        }
    }
}

std::vector<std::shared_ptr<ProducerMessage>> ProduceSet::DropPartition(const std::string &topic, int32_t partition)
{
    auto topicIt = msgs.find(topic);
    if (topicIt == msgs.end())
        return {};
    auto &partitionMap = topicIt->second;
    auto partIt = partitionMap.find(partition);
    if (partIt == partitionMap.end())
        return {};

    auto set = partIt->second;
    bufferBytes -= set->bufferBytes;
    bufferCount -= static_cast<int>(set->msgs.size());
    partitionMap.erase(partIt);
    return set->msgs;
}

bool ProduceSet::WouldOverflow(std::shared_ptr<ProducerMessage> msg)
{
    int version = Parent->conf_->Version.IsAtLeast(V0_11_0_0) ? 2 : 1;

    if (bufferBytes + msg->ByteSize(version) >= MaxRequestSize - 10 * 1024)
    {
        return true;
    }

    auto topicIt = msgs.find(msg->Topic);
    if (topicIt != msgs.end())
    {
        auto &partitions = topicIt->second;
        auto partIt = partitions.find(msg->Partition);
        if (partIt != partitions.end())
        {
            if (partIt->second->bufferBytes + msg->ByteSize(version) >= Parent->conf_->Producer.MaxMessageBytes)
            {
                return true;
            }
        }
    }

    if (Parent->conf_->Producer.Flush.MaxMessages > 0 && bufferCount >= Parent->conf_->Producer.Flush.MaxMessages)
    {
        return true;
    }

    return false;
}

bool ProduceSet::ReadyToFlush()
{
    if (Empty())
        return false;
    if (Parent->conf_->Producer.Flush.Frequency == std::chrono::milliseconds(0) &&
        Parent->conf_->Producer.Flush.Bytes == 0 &&
        Parent->conf_->Producer.Flush.Messages == 0)
    {
        return true;
    }
    if (Parent->conf_->Producer.Flush.Messages > 0 && bufferCount >= Parent->conf_->Producer.Flush.Messages)
    {
        return true;
    }
    if (Parent->conf_->Producer.Flush.Bytes > 0 && bufferBytes >= Parent->conf_->Producer.Flush.Bytes)
    {
        return true;
    }
    return false;
}

bool ProduceSet::Empty() const
{
    return bufferCount == 0;
}