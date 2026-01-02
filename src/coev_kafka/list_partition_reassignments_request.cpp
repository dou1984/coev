#include "version.h"
#include "api_versions.h"
#include "list_partition_reassignments_request.h"

void ListPartitionReassignmentsRequest::setVersion(int16_t v)
{
    Version = v;
}

int ListPartitionReassignmentsRequest::encode(PEncoder &pe)
{
    pe.putInt32(static_cast<int32_t>(Timeout.count()));

    int err = pe.putArrayLength(static_cast<int32_t>(blocks.size()));
    if (err != 0)
        return err;

    for (auto &kv : blocks)
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

int ListPartitionReassignmentsRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    int32_t timeout;
    int err = pd.getInt32(timeout);
    if (err != 0)
        return err;
    Timeout = std::chrono::milliseconds(timeout);

    int32_t topicCount;
    err = pd.getArrayLength(topicCount);
    if (err != 0)
        return err;

    if (topicCount > 0)
    {
        blocks.clear();
        blocks.reserve(topicCount);

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

            blocks[topic] = std::move(partitions);
            int32_t _;
            err = pd.getEmptyTaggedFieldArray(_);
            if (err != 0)
                return err;
        }
    }
    else
    {
        blocks.clear();
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
    return Version;
}

int16_t ListPartitionReassignmentsRequest::headerVersion() const
{
    return 2; // Always uses flexible (v2+) request header
}

bool ListPartitionReassignmentsRequest::isValidVersion() const
{
    return Version == 0;
}

bool ListPartitionReassignmentsRequest::isFlexible()
{
    return isFlexibleVersion(Version);
}

bool ListPartitionReassignmentsRequest::isFlexibleVersion(int16_t /*ver*/)
{
    return true; // Version 0 is flexible (uses tagged fields)
}

KafkaVersion ListPartitionReassignmentsRequest::requiredVersion() const
{
    return V2_4_0_0;
}

void ListPartitionReassignmentsRequest::AddBlock(const std::string &topic, const std::vector<int32_t> &partitionIDs)
{
    blocks[topic] = partitionIDs; // overwrite if exists, matching Go semantics
}