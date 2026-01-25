#include "version.h"
#include "produce_set.h"
#include "async_producer.h"
#include "producer_message.h"
#include "records.h"
#include "record_batch.h"
#include "message_set.h"
#include "produce_request.h"

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
#include "message.h"

ProduceSet::ProduceSet(std::shared_ptr<AsyncProducer> parent)
    : m_parent(parent)
{
    auto [pid, epoch] = parent->m_txnmgr->GetProducerID();
    m_producer_id = pid;
    m_producer_epoch = epoch;
}

int ProduceSet::add(std::shared_ptr<ProducerMessage> msg)
{
    std::string key, val;
    if (msg->m_key != nullptr)
    {
        if (int err = msg->m_key->Encode(key); err != 0)
            return err;
    }
    if (msg->m_value != nullptr)
    {
        if (int err = msg->m_value->Encode(val); err != 0)
            return err;
    }

    auto timestamp = msg->m_timestamp;
    if (timestamp == std::chrono::system_clock::time_point{})
    {
        timestamp = std::chrono::system_clock::now();
    }
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count();
    timestamp = std::chrono::system_clock::from_time_t(ms / 1000) + std::chrono::milliseconds(ms % 1000);

    auto &partitions = m_messages[msg->m_topic];

    int size = 0;
    auto setIt = partitions.find(msg->m_partition);
    std::shared_ptr<PartitionSet> set;
    bool isNewSet = false;
    if (setIt == partitions.end())
    {
        if (m_parent->m_conf->Version.IsAtLeast(V0_11_0_0))
        {
            auto batch = std::make_shared<RecordBatch>();
            batch->m_first_timestamp = timestamp;
            batch->m_version = 2;
            batch->m_codec = m_parent->m_conf->Producer.Compression;
            batch->m_compression_level = m_parent->m_conf->Producer.CompressionLevel;
            batch->m_producer_id = m_producer_id;
            batch->m_producer_epoch = m_producer_epoch;
            if (m_parent->m_conf->Producer.Idempotent)
            {
                batch->m_first_sequence = msg->m_sequence_number;
            }
            set = std::make_shared<PartitionSet>();
            set->m_records_to_send = Records::NewDefaultRecords(batch);
            size = RECORD_BATCH_OVERHEAD;
        }
        else
        {
            set = std::make_shared<PartitionSet>();
            set->m_records_to_send = Records::NewLegacyRecords(std::make_shared<MessageSet>());
        }
        partitions[msg->m_partition] = set;
        isNewSet = true;
    }
    else
    {
        set = setIt->second;
    }

    if (m_parent->m_conf->Version.IsAtLeast(V0_11_0_0))
    {
        if (m_parent->m_conf->Producer.Idempotent && msg->m_sequence_number < set->m_records_to_send->m_record_batch->m_first_sequence)
        {
            return -1; // assertion failed: message out of sequence added to a batch
        }
    }

    set->m_messages.push_back(msg);

    if (m_parent->m_conf->Version.IsAtLeast(V0_11_0_0))
    {
        size += MaximumRecordOverhead;
        auto rec = std::make_shared<Record>();
        rec->m_key = key;
        rec->m_value = val;
        rec->m_timestamp_delta = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp - set->m_records_to_send->m_record_batch->m_first_timestamp);

        size += key.size() + val.size();
        if (!msg->m_headers.empty())
        {
            rec->m_headers.reserve(msg->m_headers.size());
            for (auto &h : msg->m_headers)
            {
                auto header = std::make_shared<RecordHeader>(h);
                rec->m_headers.push_back(header);
                size += header->Key.size() + header->Value.size() + 2 * 5; // MaxVarintLen32 ~5
            }
        }
        set->m_records_to_send->m_record_batch->AddRecord(rec);
    }
    else
    {
        auto msgToSend = std::make_shared<Message>();
        msgToSend->m_codec = CompressionCodec::None;
        msgToSend->m_key = key;
        msgToSend->m_value = val;
        if (m_parent->m_conf->Version.IsAtLeast(V0_10_0_0))
        {
            msgToSend->m_timestamp.set_time(timestamp);
            msgToSend->m_version = 1;
        }
        set->m_records_to_send->m_msg_set->add_message(msgToSend);
        size = ProducerMessageOverhead + key.size() + val.size();
    }

    set->m_buffer_bytes += size;
    m_buffer_bytes += size;
    m_buffer_count++;

    return 0;
}

std::shared_ptr<ProduceRequest> ProduceSet::build_request()
{
    auto req = std::make_shared<ProduceRequest>();
    req->m_acks = m_parent->m_conf->Producer.Acks;
    req->m_timeout = m_parent->m_conf->Producer.Timeout;

    if (m_parent->m_conf->Version.IsAtLeast(V0_10_0_0))
        req->m_version = 2;
    if (m_parent->m_conf->Version.IsAtLeast(V0_11_0_0))
        req->m_version = 3;
    if (m_parent->m_conf->Version.IsAtLeast(V1_0_0_0))
        req->m_version = 5;
    if (m_parent->m_conf->Version.IsAtLeast(V2_0_0_0))
        req->m_version = 6;
    if (m_parent->m_conf->Version.IsAtLeast(V2_1_0_0))
        req->m_version = 7;

    if (m_parent->m_conf->Version.IsAtLeast(V0_11_0_0) && m_parent->is_transactional())
    {
        req->m_transactional_id = m_parent->m_conf->Producer.Transaction.ID;
    }

    for (auto &it : m_messages)
    {
        auto &topic = it.first;
        auto &partitionSets = it.second;
        for (auto &pit : partitionSets)
        {
            auto &partition = pit.first;
            auto &set = pit.second;
            if (req->m_version >= 3)
            {
                auto rb = set->m_records_to_send->m_record_batch;
                if (!rb->m_records.empty())
                {
                    rb->m_last_offset_delta = static_cast<int32_t>(rb->m_records.size() - 1);
                    std::chrono::milliseconds maxTimestampDelta(0);
                    for (size_t i = 0; i < rb->m_records.size(); ++i)
                    {
                        rb->m_records[i]->m_offset_delta = static_cast<int64_t>(i);
                        maxTimestampDelta = std::max(maxTimestampDelta,
                                                     std::chrono::duration_cast<std::chrono::milliseconds>(rb->m_records[i]->m_timestamp_delta));
                    }
                    rb->m_max_timestamp = rb->m_first_timestamp + maxTimestampDelta;
                }
                rb->m_is_transactional = m_parent->is_transactional();
                req->AddBatch(topic, partition, rb);
                continue;
            }

            if (m_parent->m_conf->Producer.Compression == CompressionCodec::None)
            {
                req->AddSet(topic, partition, set->m_records_to_send->m_msg_set);
            }
            else
            {
                if (m_parent->m_conf->Version.IsAtLeast(V0_10_0_0))
                {
                    for (size_t i = 0; i < set->m_records_to_send->m_msg_set->m_messages.size(); ++i)
                    {
                        set->m_records_to_send->m_msg_set->m_messages[i]->m_offset = static_cast<int64_t>(i);
                    }
                }

                std::string payload;
                ::encode(*(set->m_records_to_send->m_msg_set), payload);

                auto compMsg = std::make_shared<Message>();
                compMsg->m_codec = m_parent->m_conf->Producer.Compression;
                compMsg->m_compression_level = m_parent->m_conf->Producer.CompressionLevel;
                compMsg->m_key.clear();
                compMsg->m_value = payload;
                compMsg->m_set = set->m_records_to_send->m_msg_set;

                if (m_parent->m_conf->Version.IsAtLeast(V0_10_0_0))
                {
                    compMsg->m_version = 1;
                    if (!set->m_records_to_send->m_msg_set->m_messages.empty())
                    {
                        compMsg->m_timestamp = set->m_records_to_send->m_msg_set->m_messages[0]->m_msg->m_timestamp;
                    }
                }
                req->AddMessage(topic, partition, compMsg);
            }
        }
    }

    return req;
}

void ProduceSet::each_partition(const std::function<void(const std::string &, int32_t, std::shared_ptr<PartitionSet>)> &_f)
{
    for (auto &it : m_messages)
    {
        auto &topic = it.first;
        auto &partition_set = it.second;
        for (auto &pit : partition_set)
        {
            auto &partition = pit.first;
            auto &set = pit.second;
            _f(topic, partition, set);
        }
    }
}

std::vector<std::shared_ptr<ProducerMessage>> ProduceSet::drop_partition(const std::string &topic, int32_t partition)
{
    auto tit = m_messages.find(topic);
    if (tit == m_messages.end())
        return {};
    auto &partition_map = tit->second;
    auto pit = partition_map.find(partition);
    if (pit == partition_map.end())
        return {};

    auto _set = pit->second;
    m_buffer_bytes -= _set->m_buffer_bytes;
    m_buffer_count -= static_cast<int>(_set->m_messages.size());
    partition_map.erase(pit);
    return std::move(_set->m_messages);
}

bool ProduceSet::would_overflow(std::shared_ptr<ProducerMessage> msg)
{
    int version = m_parent->m_conf->Version.IsAtLeast(V0_11_0_0) ? 2 : 1;

    if (m_buffer_bytes + msg->byte_size(version) >= MaxRequestSize - 10 * 1024)
    {
        return true;
    }

    auto topicIt = m_messages.find(msg->m_topic);
    if (topicIt != m_messages.end())
    {
        auto &partitions = topicIt->second;
        auto partIt = partitions.find(msg->m_partition);
        if (partIt != partitions.end())
        {
            if (partIt->second->m_buffer_bytes + msg->byte_size(version) >= m_parent->m_conf->Producer.MaxMessageBytes)
            {
                return true;
            }
        }
    }

    if (m_parent->m_conf->Producer.Flush.MaxMessages > 0 && m_buffer_count >= m_parent->m_conf->Producer.Flush.MaxMessages)
    {
        return true;
    }

    return false;
}

bool ProduceSet::ready_to_flush()
{
    if (empty())
    {
        return false;
    }
    if (m_parent->m_conf->Producer.Flush.Frequency == std::chrono::milliseconds(0) &&
        m_parent->m_conf->Producer.Flush.Bytes == 0 &&
        m_parent->m_conf->Producer.Flush.Messages == 0)
    {
        return true;
    }
    if (m_parent->m_conf->Producer.Flush.Messages > 0 && m_buffer_count >= m_parent->m_conf->Producer.Flush.Messages)
    {
        return true;
    }
    if (m_parent->m_conf->Producer.Flush.Bytes > 0 && m_buffer_bytes >= m_parent->m_conf->Producer.Flush.Bytes)
    {
        return true;
    }
    return false;
}

bool ProduceSet::empty() const
{
    return m_buffer_count == 0;
}