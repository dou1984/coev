
#include <cstdint>
#include <memory>
#include <functional>
#include "partitioner.h"
#include "undefined.h"
#include "crc32.h"

int ManualPartitioner::Partition(std::shared_ptr<ProducerMessage> message, int32_t numPartitions, int32_t &result)
{
    result = message->Partition;
    return 0;
}

bool ManualPartitioner::RequiresConsistency()
{
    return true;
}

std::shared_ptr<Partitioner> RandomPartitioner::NewRandomPartitioner(const std::string &topic)
{
    return std::make_shared<RandomPartitioner>();
}

RandomPartitioner::RandomPartitioner() : generator_(std::chrono::system_clock::now().time_since_epoch().count())
{
}

int RandomPartitioner::Partition(std::shared_ptr<ProducerMessage> message, int32_t numPartitions, int32_t &result)
{
    std::uniform_int_distribution<int32_t> dist(0, numPartitions - 1);
    result = dist(generator_);
    return 0;
}

bool RandomPartitioner::RequiresConsistency()
{
    return false;
}

std::shared_ptr<Partitioner> RoundRobinPartitioner::NewRoundRobinPartitioner(const std::string &topic)
{
    return std::make_shared<RoundRobinPartitioner>();
}

RoundRobinPartitioner::RoundRobinPartitioner() : partition_(0)
{
}

int RoundRobinPartitioner::Partition(std::shared_ptr<ProducerMessage> message, int32_t numPartitions, int32_t &result)
{
    if (partition_ >= numPartitions)
    {
        partition_ = 0;
    }
    result = partition_;
    partition_++;
    return 0;
}

bool RoundRobinPartitioner::RequiresConsistency()
{
    return false;
}

std::shared_ptr<Partitioner> HashPartitioner::NewHashPartitioner(const std::string &topic)
{
    auto p = std::make_shared<HashPartitioner>();
    p->random_ = RandomPartitioner::NewRandomPartitioner(topic);
    p->hasher_ = fnv::New32a();

    p->reference_abs_ = false;
    p->hash_unsigned_ = false;
    return p;
}

std::shared_ptr<Partitioner> HashPartitioner::NewReferenceHashPartitioner(const std::string &topic)
{
    auto p = std::make_shared<HashPartitioner>();
    p->random_ = RandomPartitioner::NewRandomPartitioner(topic);
    p->hasher_ = fnv::New32a();
    p->reference_abs_ = true;
    p->hash_unsigned_ = false;
    return p;
}

std::shared_ptr<Partitioner> HashPartitioner::NewConsistentCRCHashPartitioner(const std::string &topic)
{
    auto p = std::make_shared<HashPartitioner>();
    p->random_ = RandomPartitioner::NewRandomPartitioner(topic);
    p->hasher_ = std::make_shared<CRC32>();
    p->reference_abs_ = false;
    p->hash_unsigned_ = true;
    return p;
}

PartitionerConstructor HashPartitioner::NewCustomHashPartitioner(std::function<std::shared_ptr<Hash32>()> hasher)
{
    return [hasher](const std::string &topic) -> std::shared_ptr<Partitioner>
    {
        auto p = std::make_shared<HashPartitioner>();
        p->random_ = RandomPartitioner::NewRandomPartitioner(topic);
        p->hasher_ = hasher();
        p->reference_abs_ = false;
        p->hash_unsigned_ = false;
        return p;
    };
}

PartitionerConstructor HashPartitioner::NewCustomPartitioner(const std::vector<std::shared_ptr<HashPartitionerOption>> &options)
{
    return [options](const std::string &topic) -> std::shared_ptr<Partitioner>
    {
        auto p = std::make_shared<HashPartitioner>();
        p->random_ = RandomPartitioner::NewRandomPartitioner(topic);
        p->hasher_ = fnv::New32a();
        p->reference_abs_ = false;
        p->hash_unsigned_ = false;

        for (auto &option : options)
        {
            option->Apply(p.get());
        }

        return p;
    };
}

HashPartitioner::HashPartitioner() : reference_abs_(false), hash_unsigned_(false)
{
}

int HashPartitioner::Partition(std::shared_ptr<ProducerMessage> message, int32_t numPartitions, int32_t &result)
{
    if (!message->Key)
    {
        return random_->Partition(message, numPartitions, result);
    }
    std::string bytes;
    int err = message->Key->Encode(bytes);
    if (err != 0)
    {
        return -1;
    }

    hasher_->Reset();
    hasher_->Update(bytes.data(), bytes.size());
    uint32_t hash_value = hasher_->Final();

    if (reference_abs_)
    {
        result = (static_cast<int32_t>(hash_value) & 0x7fffffff) % numPartitions;
    }
    else if (hash_unsigned_)
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
    return static_cast<bool>(message->Key);
}

void WithAbsFirstOption::Apply(HashPartitioner *hp)
{
    hp->reference_abs_ = true;
}

void WithHashUnsignedOption::Apply(HashPartitioner *hp)
{
    hp->hash_unsigned_ = true;
}

WithCustomHashFunctionOption::WithCustomHashFunctionOption(std::function<std::shared_ptr<Hash32>()> hasher) : hasher_(hasher)
{
}

void WithCustomHashFunctionOption::Apply(HashPartitioner *hp)
{
    hp->hasher_ = hasher_();
}

WithCustomFallbackPartitionerOption::WithCustomFallbackPartitionerOption(std::shared_ptr<Partitioner> randomHP) : random_(randomHP)
{
}

void WithCustomFallbackPartitionerOption::Apply(HashPartitioner *hp)
{
    hp->random_ = random_;
}