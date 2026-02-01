#include "sticky_assignor_user_data.h"
#include <string>
#include <vector>
#include <map>

const int defaultGeneration = 0;

int StickyAssignorUserDataV0::encode(packet_encoder &pe)
{
    if (int err = pe.putArrayLength(static_cast<int>(m_topics.size())); err != 0)
    {
        return err;
    }

    for (auto &kv : m_topics)
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

int StickyAssignorUserDataV0::decode(packet_decoder &pd)
{
    int topicLen = 0;
    int err = pd.getArrayLength(topicLen);
    if (err != 0)
    {
        return err;
    }

    m_topics.clear();
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
        m_topics[topic] = std::move(partitions);
    }
    m_topic_partitions = PopulateTopicPartitions(m_topics);
    return 0;
}

std::vector<TopicPartitionAssignment> StickyAssignorUserDataV0::partitions()
{
    return m_topic_partitions;
}

bool StickyAssignorUserDataV0::hasGeneration()
{
    return false;
}

int StickyAssignorUserDataV0::generation()
{
    return defaultGeneration;
}

int StickyAssignorUserDataV1::encode(packet_encoder &pe)
{
    if (int err = pe.putArrayLength(static_cast<int>(m_topics.size())); err != 0)
    {
        return err;
    }

    for (auto &kv : m_topics)
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

    pe.putInt32(m_generation);
    return 0;
}

int StickyAssignorUserDataV1::decode(packet_decoder &pd)
{
    int topicLen = 0;
    int err = pd.getArrayLength(topicLen);
    if (err != 0)
    {
        return err;
    }

    m_topics.clear();
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
        m_topics[topic] = std::move(partitions);
    }

    if (int e = pd.getInt32(m_generation); e != 0)
    {
        return e;
    }
    m_topic_partitions = PopulateTopicPartitions(m_topics);
    return 0;
}

std::vector<TopicPartitionAssignment> StickyAssignorUserDataV1::partitions()
{
    return m_topic_partitions;
}

bool StickyAssignorUserDataV1::hasGeneration()
{
    return true;
}

int StickyAssignorUserDataV1::generation()
{
    return static_cast<int>(m_generation);
}

std::vector<TopicPartitionAssignment> PopulateTopicPartitions(const std::map<std::string, std::vector<int32_t>> &topics)
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