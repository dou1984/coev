
#include <cstdint>
#include <memory>
#include <functional>
#include "partitioner.h"
#include "undefined.h"
#include "../utils/hash/fnv.h"
#include "../utils/hash/crc32.h"

int ManualPartitioner::Partition(std::shared_ptr<ProducerMessage> message, int32_t numPartitions, int32_t &result)
{
    result = message->m_partition;
    return 0;
}

bool ManualPartitioner::RequiresConsistency()
{
    return true;
}

std::shared_ptr<Partitioner> NewRandomPartitioner(const std::string &topic)
{
    return std::make_shared<RandomPartitioner>();
}

RandomPartitioner::RandomPartitioner() : m_generator(std::chrono::system_clock::now().time_since_epoch().count())
{
}

int RandomPartitioner::Partition(std::shared_ptr<ProducerMessage> message, int32_t numPartitions, int32_t &result)
{
    std::uniform_int_distribution<int32_t> dist(0, numPartitions - 1);
    result = dist(m_generator);
    return 0;
}

bool RandomPartitioner::RequiresConsistency()
{
    return false;
}

std::shared_ptr<Partitioner> NewRoundRobinPartitioner(const std::string &topic)
{
    return std::make_shared<RoundRobinPartitioner>();
}

RoundRobinPartitioner::RoundRobinPartitioner() : m_partition(0)
{
}

int RoundRobinPartitioner::Partition(std::shared_ptr<ProducerMessage> message, int32_t numPartitions, int32_t &result)
{
    if (m_partition >= numPartitions)
    {
        m_partition = 0;
    }
    result = m_partition;
    m_partition++;
    return 0;
}

bool RoundRobinPartitioner::RequiresConsistency()
{
    return false;
}

HashPartitioner::HashPartitioner() : m_reference_abs(false), m_hash_unsigned(false)
{
}

int HashPartitioner::Partition(std::shared_ptr<ProducerMessage> message, int32_t numPartitions, int32_t &result)
{
    if (!message->m_key)
    {
        return m_random_partitioner->Partition(message, numPartitions, result);
    }
    std::string bytes;
    int err = message->m_key->Encode(bytes);
    if (err != 0)
    {
        return -1;
    }

    m_hasher->Reset();
    m_hasher->Update(bytes.data(), bytes.size());
    uint32_t hash_value = m_hasher->Final();

    if (m_reference_abs)
    {
        result = (static_cast<int32_t>(hash_value) & 0x7fffffff) % numPartitions;
    }
    else if (m_hash_unsigned)
    {
        result = static_cast<int32_t>(hash_value % static_cast<uint32_t>(numPartitions));
    }
    else
    {
        result = static_cast<int32_t>(hash_value) % numPartitions;
        if (result < 0)
        {
            result = -result;
        }
    }

    return 0;
}

bool HashPartitioner::RequiresConsistency()
{
    return true;
}

bool HashPartitioner::MessageRequiresConsistency(std::shared_ptr<ProducerMessage> message)
{
    return static_cast<bool>(message->m_key);
}

void WithAbsFirstOption::Apply(HashPartitioner *hp)
{
    hp->m_reference_abs = true;
}

void WithHashUnsignedOption::Apply(HashPartitioner *hp)
{
    hp->m_hash_unsigned = true;
}

WithCustomHashFunctionOption::WithCustomHashFunctionOption(std::function<std::shared_ptr<Hash32>()> hasher) : m_hasher(hasher)
{
}

void WithCustomHashFunctionOption::Apply(HashPartitioner *hp)
{
    hp->m_hasher = m_hasher();
}

WithCustomFallbackPartitionerOption::WithCustomFallbackPartitionerOption(std::shared_ptr<Partitioner> randomHP) : m_random(randomHP)
{
}

void WithCustomFallbackPartitionerOption::Apply(HashPartitioner *hp)
{
    hp->m_random_partitioner = m_random;
}

std::shared_ptr<Partitioner> NewHashPartitioner(const std::string &topic)
{
    auto p = std::make_shared<HashPartitioner>();
    p->m_random_partitioner = NewRandomPartitioner(topic);
    p->m_hasher = coev::fnv::New32a();

    p->m_reference_abs = false;
    p->m_hash_unsigned = false;
    return p;
}

std::shared_ptr<Partitioner> NewReferenceHashPartitioner(const std::string &topic)
{
    auto p = std::make_shared<HashPartitioner>();
    p->m_random_partitioner = NewRandomPartitioner(topic);
    p->m_hasher = coev::fnv::New32a();
    p->m_reference_abs = true;
    p->m_hash_unsigned = false;
    return p;
}

std::shared_ptr<Partitioner> NewConsistentCRCHashPartitioner(const std::string &topic)
{
    auto p = std::make_shared<HashPartitioner>();
    p->m_random_partitioner = NewRandomPartitioner(topic);
    p->m_hasher = coev::crc32::NewIEEE();
    p->m_reference_abs = false;
    p->m_hash_unsigned = true;
    return p;
}

PartitionerConstructor NewCustomHashPartitioner(std::function<std::shared_ptr<Hash32>()> hasher)
{
    return [hasher](const std::string &topic) -> std::shared_ptr<Partitioner>
    {
        auto p = std::make_shared<HashPartitioner>();
        p->m_random_partitioner = NewRandomPartitioner(topic);
        p->m_hasher = hasher();
        p->m_reference_abs = false;
        p->m_hash_unsigned = false;
        return p;
    };
}

PartitionerConstructor NewCustomPartitioner(const std::vector<std::shared_ptr<HashPartitionerOption>> &options)
{
    return [options](const std::string &topic) -> std::shared_ptr<Partitioner>
    {
        auto p = std::make_shared<HashPartitioner>();
        p->m_random_partitioner = NewRandomPartitioner(topic);
        p->m_hasher = coev::fnv::New32a();
        p->m_reference_abs = false;
        p->m_hash_unsigned = false;

        for (auto &option : options)
        {
            option->Apply(p.get());
        }

        return p;
    };
}
