#include "list_partition_reassignments_response.h"
#include "api_versions.h"

int PartitionReplicaReassignmentsStatus::encode(PEncoder &pe)
{
    int err = pe.putInt32Array(Replicas);
    if (err != 0)
        return err;

    err = pe.putInt32Array(AddingReplicas);
    if (err != 0)
        return err;

    err = pe.putInt32Array(RemovingReplicas);
    if (err != 0)
        return err;

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int PartitionReplicaReassignmentsStatus::decode(PDecoder &pd)
{
    int err = pd.getInt32Array(Replicas);
    if (err != 0)
        return err;

    err = pd.getInt32Array(AddingReplicas);
    if (err != 0)
        return err;

    err = pd.getInt32Array(RemovingReplicas);
    if (err != 0)
        return err;
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

// ---------- ListPartitionReassignmentsResponse ----------

void ListPartitionReassignmentsResponse::setVersion(int16_t v)
{
    Version = v;
}

void ListPartitionReassignmentsResponse::AddBlock(
    const std::string &topic,
    int32_t partition,
    const std::vector<int32_t> &replicas,
    const std::vector<int32_t> &addingReplicas,
    const std::vector<int32_t> &removingReplicas)
{

    auto &partitions = TopicStatus[topic];
    auto block = std::make_unique<PartitionReplicaReassignmentsStatus>();
    block->Replicas = replicas;
    block->AddingReplicas = addingReplicas;
    block->RemovingReplicas = removingReplicas;
    partitions[partition] = std::move(block);
}

int ListPartitionReassignmentsResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(ThrottleTime);
    pe.putKError(ErrorCode);
    int err = pe.putNullableString(ErrorMessage);
    if (err != 0)
        return err;

    err = pe.putArrayLength(static_cast<int32_t>(TopicStatus.size()));
    if (err != 0)
        return err;

    for (auto &topicEntry : TopicStatus)
    {
        auto &topic = topicEntry.first;
        auto &partitions = topicEntry.second;

        err = pe.putString(topic);
        if (err != 0)
            return err;

        err = pe.putArrayLength(static_cast<int32_t>(partitions.size()));
        if (err != 0)
            return err;

        for (auto &partEntry : partitions)
        {
            int32_t partition = partEntry.first;
            auto &block = partEntry.second;

            pe.putInt32(partition);
            err = block->encode(pe);
            if (err != 0)
                return err;
        }

        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int ListPartitionReassignmentsResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    int err = pd.getDurationMs(ThrottleTime);
    if (err != 0)
        return err;

    err = pd.getKError(ErrorCode);
    if (err != 0)
        return err;

    err = pd.getNullableString(ErrorMessage);
    if (err != 0)
        return err;

    int32_t numTopics;
    err = pd.getArrayLength(numTopics);
    if (err != 0)
        return err;

    TopicStatus.clear();

    for (int32_t i = 0; i < numTopics; ++i)
    {
        std::string topic;
        err = pd.getString(topic);
        if (err != 0)
            return err;

        int32_t numPartitions;
        err = pd.getArrayLength(numPartitions);
        if (err != 0)
            return err;

        auto &partitionsMap = TopicStatus[topic];

        for (int32_t j = 0; j < numPartitions; ++j)
        {
            int32_t partition;
            err = pd.getInt32(partition);
            if (err != 0)
                return err;

            auto block = std::make_unique<PartitionReplicaReassignmentsStatus>();
            err = block->decode(pd);
            if (err != 0)
                return err;

            partitionsMap[partition] = std::move(block);
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
    return Version;
}

int16_t ListPartitionReassignmentsResponse::headerVersion() const
{
    return 1; // flexible response header starts at version 1
}

bool ListPartitionReassignmentsResponse::isValidVersion() const
{
    return Version == 0;
}

bool ListPartitionReassignmentsResponse::isFlexible()
{
    return isFlexibleVersion(Version);
}

bool ListPartitionReassignmentsResponse::isFlexibleVersion(int16_t /*ver*/)
{
    return true; // version 0 uses flexible format
}

KafkaVersion ListPartitionReassignmentsResponse::requiredVersion() const
{
    return V2_4_0_0;
}

std::chrono::milliseconds ListPartitionReassignmentsResponse::throttleTime() const
{
    return ThrottleTime;
}