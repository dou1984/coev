#include "sticky_assignor_user_data.h"
#include <string>
#include <vector>
#include <map>

const int defaultGeneration = 0;

int StickyAssignorUserDataV0::encode(PEncoder &pe)
{
    if (int err = pe.putArrayLength(static_cast<int>(Topics.size())); err != 0)
    {
        return err;
    }

    for (auto &kv : Topics)
    {
        if (int err = pe.putString(kv.first); err != 0)
        {
            return err;
        }
        if (int err = pe.putInt32Array(kv.second); err != 0)
        {
            return err;
        }
    }
    return 0;
}

int StickyAssignorUserDataV0::decode(PDecoder &pd)
{
    int topicLen = 0;
    int err = pd.getArrayLength(topicLen);
    if (err != 0)
    {
        return err;
    }

    Topics.clear();
    for (int i = 0; i < topicLen; ++i)
    {
        std::string topic;
        if (int e = pd.getString(topic); e != 0)
        {
            return e;
        }
        std::vector<int32_t> partitions;
        if (int e = pd.getInt32Array(partitions); e != 0)
        {
            return e;
        }
        Topics[topic] = std::move(partitions);
    }
    topicPartitions = populateTopicPartitions(Topics);
    return 0;
}

std::vector<TopicPartitionAssignment> StickyAssignorUserDataV0::partitions()
{
    return topicPartitions;
}

bool StickyAssignorUserDataV0::hasGeneration()
{
    return false;
}

int StickyAssignorUserDataV0::generation()
{
    return defaultGeneration;
}

int StickyAssignorUserDataV1::encode(PEncoder &pe)
{
    if (int err = pe.putArrayLength(static_cast<int>(Topics.size())); err != 0)
    {
        return err;
    }

    for (auto &kv : Topics)
    {
        if (int err = pe.putString(kv.first); err != 0)
        {
            return err;
        }
        if (int err = pe.putInt32Array(kv.second); err != 0)
        {
            return err;
        }
    }

    pe.putInt32(Generation);
    return 0;
}

int StickyAssignorUserDataV1::decode(PDecoder &pd)
{
    int topicLen = 0;
    int err = pd.getArrayLength(topicLen);
    if (err != 0)
    {
        return err;
    }

    Topics.clear();
    for (int i = 0; i < topicLen; ++i)
    {
        std::string topic;
        if (int e = pd.getString(topic); e != 0)
        {
            return e;
        }
        std::vector<int32_t> partitions;
        if (int e = pd.getInt32Array(partitions); e != 0)
        {
            return e;
        }
        Topics[topic] = std::move(partitions);
    }

    if (int e = pd.getInt32(Generation); e != 0)
    {
        return e;
    }
    topicPartitions = populateTopicPartitions(Topics);
    return 0;
}

std::vector<TopicPartitionAssignment> StickyAssignorUserDataV1::partitions()
{
    return topicPartitions;
}

bool StickyAssignorUserDataV1::hasGeneration()
{
    return true;
}

int StickyAssignorUserDataV1::generation()
{
    return static_cast<int>(Generation);
}

std::vector<TopicPartitionAssignment> populateTopicPartitions(const std::unordered_map<std::string, std::vector<int32_t>> &topics)
{
    std::vector<TopicPartitionAssignment> topicPartitions;
    for (auto &kv : topics)
    {
        const std::string &topic = kv.first;
        const std::vector<int32_t> &partitions = kv.second;
        for (int32_t partition : partitions)
        {
            topicPartitions.emplace_back(topic, partition);
        }
    }
    return topicPartitions;
}