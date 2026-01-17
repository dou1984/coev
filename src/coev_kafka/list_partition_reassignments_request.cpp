#include "version.h"
#include "api_versions.h"
#include "list_partition_reassignments_request.h"

void ListPartitionReassignmentsRequest::set_version(int16_t v)
{
    m_version = v;
}

int ListPartitionReassignmentsRequest::encode(packetEncoder &pe)
{
    pe.putInt32(static_cast<int32_t>(m_timeout.count()));

    int err = pe.putArrayLength(static_cast<int32_t>(m_blocks.size()));
    if (err != 0)
        return err;

    for (auto &kv : m_blocks)
    {
        const std::string &topic = kv.first;
        const std::vector<int32_t> &partitions = kv.second;

        err = pe.putString(topic);
        if (err != 0)
            return err;

        err = pe.putInt32Array(partitions);
        if (err != 0)
            return err;

        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int ListPartitionReassignmentsRequest::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;

    int32_t timeout;
    int err = pd.getInt32(timeout);
    if (err != 0)
        return err;
    m_timeout = std::chrono::milliseconds(timeout);

    int32_t topicCount;
    err = pd.getArrayLength(topicCount);
    if (err != 0)
        return err;

    if (topicCount > 0)
    {
        m_blocks.clear();
        m_blocks.reserve(topicCount);

        for (int32_t i = 0; i < topicCount; ++i)
        {
            std::string topic;
            err = pd.getString(topic);
            if (err != 0)
                return err;

            int32_t partitionCount;
            err = pd.getArrayLength(partitionCount);
            if (err != 0)
                return err;

            std::vector<int32_t> partitions(partitionCount);
            for (int32_t j = 0; j < partitionCount; ++j)
            {
                err = pd.getInt32(partitions[j]);
                if (err != 0)
                    return err;
            }

            m_blocks[topic] = std::move(partitions);
            int32_t _;
            err = pd.getEmptyTaggedFieldArray(_);
            if (err != 0)
                return err;
        }
    }
    else
    {
        m_blocks.clear();
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t ListPartitionReassignmentsRequest::key() const
{
    return apiKeyListPartitionReassignments;
}

int16_t ListPartitionReassignmentsRequest::version() const
{
    return m_version;
}

int16_t ListPartitionReassignmentsRequest::header_version() const
{
    return 2; // Always uses flexible (v2+) request header
}

bool ListPartitionReassignmentsRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

bool ListPartitionReassignmentsRequest::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool ListPartitionReassignmentsRequest::is_flexible_version(int16_t /*ver*/) const
{
    return true; // Version 0 is flexible (uses tagged fields)
}

KafkaVersion ListPartitionReassignmentsRequest::required_version() const
{
    return V2_4_0_0;
}

void ListPartitionReassignmentsRequest::AddBlock(const std::string &topic, const std::vector<int32_t> &partitionIDs)
{
    m_blocks[topic] = partitionIDs; // overwrite if exists, matching Go semantics
}