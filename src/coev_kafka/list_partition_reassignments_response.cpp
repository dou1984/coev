#include "list_partition_reassignments_response.h"
#include "api_versions.h"

int PartitionReplicaReassignmentsStatus::encode(packet_encoder &pe) const
{
    int err = pe.putInt32Array(m_replicas);
    if (err != 0)
        return err;

    err = pe.putInt32Array(m_adding_replicas);
    if (err != 0)
        return err;

    err = pe.putInt32Array(m_removing_replicas);
    if (err != 0)
        return err;

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int PartitionReplicaReassignmentsStatus::decode(packet_decoder &pd)
{
    int err = pd.getInt32Array(m_replicas);
    if (err != 0)
        return err;

    err = pd.getInt32Array(m_adding_replicas);
    if (err != 0)
        return err;

    err = pd.getInt32Array(m_removing_replicas);
    if (err != 0)
        return err;
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

void ListPartitionReassignmentsResponse::set_version(int16_t v)
{
    m_version = v;
}

void ListPartitionReassignmentsResponse::add_block(const std::string &topic, int32_t partition, const std::vector<int32_t> &replicas, const std::vector<int32_t> &addingReplicas, const std::vector<int32_t> &removingReplicas)
{
    auto &block = m_topic_status[topic][partition];
    block.m_replicas = replicas;
    block.m_adding_replicas = addingReplicas;
    block.m_removing_replicas = removingReplicas;
}

int ListPartitionReassignmentsResponse::encode(packet_encoder &pe) const
{
    pe.putDurationMs(m_throttle_time);
    pe.putKError(m_err);
    int err = pe.putNullableString(m_message);
    if (err != 0)
        return err;

    err = pe.putArrayLength(static_cast<int32_t>(m_topic_status.size()));
    if (err != 0)
        return err;

    for (auto &[topic, partitions] : m_topic_status)
    {

        err = pe.putString(topic);
        if (err != 0)
            return err;

        err = pe.putArrayLength(static_cast<int32_t>(partitions.size()));
        if (err != 0)
            return err;

        for (auto &[partition, block] : partitions)
        {
            pe.putInt32(partition);
            err = block.encode(pe);
            if (err != 0)
                return err;
        }

        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int ListPartitionReassignmentsResponse::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;

    int err = pd.getDurationMs(m_throttle_time);
    if (err != 0)
        return err;

    err = pd.getKError(m_err);
    if (err != 0)
        return err;

    err = pd.getNullableString(m_message);
    if (err != 0)
        return err;

    int32_t num_topics;
    err = pd.getArrayLength(num_topics);
    if (err != 0)
        return err;

    m_topic_status.clear();

    for (int32_t i = 0; i < num_topics; ++i)
    {
        std::string topic;
        err = pd.getString(topic);
        if (err != 0)
            return err;

        int32_t num_partitions;
        err = pd.getArrayLength(num_partitions);
        if (err != 0)
            return err;

        auto &partitions = m_topic_status[topic];

        for (int32_t j = 0; j < num_partitions; ++j)
        {
            int32_t partition;
            err = pd.getInt32(partition);
            if (err != 0)
                return err;

            err = partitions[partition].decode(pd);
            if (err != 0)
                return err;
        }
        int32_t _;
        err = pd.getEmptyTaggedFieldArray(_);
        if (err != 0)
            return err;
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t ListPartitionReassignmentsResponse::key() const
{
    return apiKeyListPartitionReassignments;
}

int16_t ListPartitionReassignmentsResponse::version() const
{
    return m_version;
}

int16_t ListPartitionReassignmentsResponse::header_version() const
{
    return 1; // flexible response header starts at version 1
}

bool ListPartitionReassignmentsResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

bool ListPartitionReassignmentsResponse::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool ListPartitionReassignmentsResponse::is_flexible_version(int16_t /*ver*/) const
{
    return true; // version 0 uses flexible format
}

KafkaVersion ListPartitionReassignmentsResponse::required_version() const
{
    return V2_4_0_0;
}

std::chrono::milliseconds ListPartitionReassignmentsResponse::throttle_time() const
{
    return m_throttle_time;
}