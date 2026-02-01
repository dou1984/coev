#pragma once

#include <memory>
#include <string>
#include <functional>
#include <random>
#include <chrono>
#include "config.h"
#include "producer_message.h"
#include "undefined.h"

struct Partitioner
{
    virtual ~Partitioner() = default;
    virtual int partition(std::shared_ptr<ProducerMessage> message, int32_t numPartitions, int32_t &result) = 0;
    virtual bool requires_consistency() = 0;
};

struct DynamicConsistencyPartitioner : Partitioner
{
    virtual bool message_requires_consistency(std::shared_ptr<ProducerMessage> message) = 0;
};

struct HashPartitionerOption
{
    virtual void apply(class HashPartitioner *hp) = 0;
};

struct ManualPartitioner : Partitioner
{
    int partition(std::shared_ptr<ProducerMessage> message, int32_t numPartitions, int32_t &result);
    bool requires_consistency();
};

struct RandomPartitioner : Partitioner
{
    int partition(std::shared_ptr<ProducerMessage> message, int32_t numPartitions, int32_t &result);
    bool requires_consistency();

    RandomPartitioner();
    std::mt19937 m_generator;
};

struct RoundRobinPartitioner : Partitioner
{

    RoundRobinPartitioner();
    int partition(std::shared_ptr<ProducerMessage> message, int32_t numPartitions, int32_t &result);
    bool requires_consistency();

    int32_t m_partition;
};

struct HashPartitioner : DynamicConsistencyPartitioner
{
    HashPartitioner();
    int partition(std::shared_ptr<ProducerMessage> message, int32_t numPartitions, int32_t &result);
    bool requires_consistency();
    bool message_requires_consistency(std::shared_ptr<ProducerMessage> message);

    std::shared_ptr<Partitioner> m_random_partitioner;
    std::shared_ptr<Hash32> m_hasher;
    bool m_reference_abs;
    bool m_hash_unsigned;
};

struct WithAbsFirstOption : HashPartitionerOption
{
    void apply(HashPartitioner *hp);
};

struct WithHashUnsignedOption : HashPartitionerOption
{
    void apply(HashPartitioner *hp);
};

struct WithCustomHashFunctionOption : HashPartitionerOption
{
    WithCustomHashFunctionOption(std::function<std::shared_ptr<Hash32>()> hasher);
    void apply(HashPartitioner *hp);
    std::function<std::shared_ptr<Hash32>()> m_hasher;
};

struct WithCustomFallbackPartitionerOption : HashPartitionerOption
{
    WithCustomFallbackPartitionerOption(std::shared_ptr<Partitioner> randomHP);
    void apply(HashPartitioner *hp);
    std::shared_ptr<Partitioner> m_random;
};

std::shared_ptr<Partitioner> NewRandomPartitioner(const std::string &topic);
std::shared_ptr<Partitioner> NewHashPartitioner(const std::string &topic);
std::shared_ptr<Partitioner> NewReferenceHashPartitioner(const std::string &topic);
std::shared_ptr<Partitioner> NewConsistentCRCHashPartitioner(const std::string &topic);
std::shared_ptr<Partitioner> NewRoundRobinPartitioner(const std::string &topic);
PartitionerConstructor NewCustomHashPartitioner(std::function<std::shared_ptr<Hash32>()> hasher);
PartitionerConstructor NewCustomPartitioner(const std::vector<std::shared_ptr<HashPartitionerOption>> &options);