#include "version.h"
#include "produce_set.h"
#include "async_producer.h"
#include "producer_message.h"
#include "records.h"
#include "record_batch.h"
#include "message_set.h"
#include "produce_request.h"
#include "records.h"

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
    auto [pid, epoch] = parent->m_txnmgr->get_producer_id();
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

    auto pset = std::make_shared<PartitionSet>();
    bool isNewSet = false;
    auto it = partitions.find(msg->m_partition);
    if (it == partitions.end())
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
            pset->m_records = std::make_shared<Records>(batch);
            size = RECORD_BATCH_OVERHEAD;
        }
        else
        {
            pset->m_records = std::make_shared<Records>(std::make_shared<MessageSet>());
        }
        partitions[msg->m_partition] = pset;
        isNewSet = true;
    }
    else
    {
        pset = it->second;
    }

    if (m_parent->m_conf->Version.IsAtLeast(V0_11_0_0))
    {
        if (m_parent->m_conf->Producer.Idempotent && msg->m_sequence_number < pset->m_records->m_record_batch->m_first_sequence)
        {
            return -1; // assertion failed: message out of sequence added to a batch
        }
    }

    pset->m_messages.push_back(msg);

    if (m_parent->m_conf->Version.IsAtLeast(V0_11_0_0))
    {
        size += MaximumRecordOverhead;
        auto rec = std::make_shared<Record>();
        rec->m_key = key;
        rec->m_value = val;
        rec->m_timestamp_delta = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp - pset->m_records->m_record_batch->m_first_timestamp);

        size += key.size() + val.size();
        if (!msg->m_headers.empty())
        {
            rec->m_headers.reserve(msg->m_headers.size());
            for (auto &h : msg->m_headers)
            {
                rec->m_headers.emplace_back(h.m_key, h.m_value);
                size += h.m_key.size() + h.m_value.size() + 2 * 5; // MaxVarintLen32 ~5
            }
        }
        pset->m_records->m_record_batch->add_record(rec);
    }
    else
    {
        auto msg_to_send = std::make_shared<Message>();
        msg_to_send->m_codec = CompressionCodec::None;
        msg_to_send->m_key = key;
        msg_to_send->m_value = val;
        if (m_parent->m_conf->Version.IsAtLeast(V0_10_0_0))
        {
            msg_to_send->m_timestamp.set_time(timestamp);
            msg_to_send->m_version = 1;
        }
        pset->m_records->m_message_set->add_message(msg_to_send);
        size = ProducerMessageOverhead + key.size() + val.size();
    }

    pset->m_buffer_bytes += size;
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

    for (auto &[topic, partitions] : m_messages)
    {
        for (auto &[partition, pset] : partitions)
        {
            if (req->m_version >= 3)
            {
                auto rb = pset->m_records->m_record_batch;
                if (!rb->m_records.empty())
                {
                    rb->m_last_offset_delta = static_cast<int32_t>(rb->m_records.size() - 1);
                    std::chrono::milliseconds MaxTimestampDelta(0);
                    for (size_t i = 0; i < rb->m_records.size(); ++i)
                    {
                        rb->m_records[i]->m_offset_delta = static_cast<int64_t>(i);
                        MaxTimestampDelta = std::max(MaxTimestampDelta, std::chrono::duration_cast<std::chrono::milliseconds>(rb->m_records[i]->m_timestamp_delta));
                    }
                    rb->m_max_timestamp = rb->m_first_timestamp + MaxTimestampDelta;
                }
                rb->m_is_transactional = m_parent->is_transactional();
                req->add_batch(topic, partition, rb);
                continue;
            }

            if (m_parent->m_conf->Producer.Compression == CompressionCodec::None)
            {
                req->add_set(topic, partition, pset->m_records->m_message_set);
            }
            else
            {
                if (m_parent->m_conf->Version.IsAtLeast(V0_10_0_0))
                {
                    for (size_t i = 0; i < pset->m_records->m_message_set->m_messages.size(); ++i)
                    {
                        pset->m_records->m_message_set->m_messages[i].m_offset = static_cast<int64_t>(i);
                    }
                }

                std::string payload;
                ::encode(*(pset->m_records->m_message_set), payload);

                auto comp_msg = std::make_shared<Message>();
                comp_msg->m_codec = m_parent->m_conf->Producer.Compression;
                comp_msg->m_compression_level = m_parent->m_conf->Producer.CompressionLevel;
                comp_msg->m_key.clear();
                comp_msg->m_value = std::move(payload);
                comp_msg->m_message_set = *pset->m_records->m_message_set;

                if (m_parent->m_conf->Version.IsAtLeast(V0_10_0_0))
                {
                    comp_msg->m_version = 1;
                    if (!pset->m_records->m_message_set->m_messages.empty())
                    {
                        comp_msg->m_timestamp = pset->m_records->m_message_set->m_messages[0].m_message->m_timestamp;
                    }
                }
                req->add_message(topic, partition, comp_msg);
            }
        }
    }

    return req;
}

std::vector<std::shared_ptr<ProducerMessage>> ProduceSet::drop_partition(const std::string &topic, int32_t partition)
{
    auto tit = m_messages.find(topic);
    if (tit == m_messages.end())
    {
        return {};
    }
    auto &partitions = tit->second;
    auto pit = partitions.find(partition);
    if (pit == partitions.end())
    {
        return {};
    }

    auto pset = pit->second;
    m_buffer_bytes -= pset->m_buffer_bytes;
    m_buffer_count -= static_cast<int>(pset->m_messages.size());
    partitions.erase(pit);
    return std::move(pset->m_messages);
}

bool ProduceSet::would_overflow(std::shared_ptr<ProducerMessage> msg)
{
    int version = m_parent->m_conf->Version.IsAtLeast(V0_11_0_0) ? 2 : 1;

    if (m_buffer_bytes + msg->byte_size(version) >= MaxRequestSize - 10 * 1024)
    {
        return true;
    }

    auto tit = m_messages.find(msg->m_topic);
    if (tit != m_messages.end())
    {
        auto &partitions = tit->second;
        auto pit = partitions.find(msg->m_partition);
        if (pit != partitions.end())
        {
            if (pit->second->m_buffer_bytes + msg->byte_size(version) >= m_parent->m_conf->Producer.MaxMessageBytes)
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