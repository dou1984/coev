#include <gtest/gtest.h>
#include <vector>
#include <cstdint>
#include <string>
#include <memory>

#include "partitioner.h"
#include "producer_message.h"
#include "utils.h"

// Helper function to assert consistent partitioning
void assertPartitioningConsistent(Partitioner *partitioner, std::shared_ptr<ProducerMessage> message, int32_t numPartitions)
{
    int32_t choice = 0;
    int result = partitioner->partition(message, numPartitions, choice);
    ASSERT_EQ(result, 0) << "Partitioning failed";
    ASSERT_GE(choice, 0) << "Partition out of range";
    ASSERT_LT(choice, numPartitions) << "Partition out of range";

    for (int i = 1; i < 50; ++i)
    {
        int32_t newChoice = 0;
        result = partitioner->partition(message, numPartitions, newChoice);
        ASSERT_EQ(result, 0) << "Partitioning failed";
        ASSERT_EQ(newChoice, choice) << "Inconsistent partitioning";
    }
}

struct PartitionerTestCase
{
    std::string key;
    int32_t expectedPartition;
};

void partitionAndAssert(Partitioner *partitioner, int32_t numPartitions, const PartitionerTestCase &testCase)
{
    auto message = std::make_shared<ProducerMessage>();
    message->m_key = std::make_shared<StringEncoder>(testCase.key);

    int32_t choice = 0;
    int result = partitioner->partition(message, numPartitions, choice);
    ASSERT_EQ(result, 0) << "Partitioning failed for key: " << testCase.key;
    ASSERT_EQ(choice, testCase.expectedPartition) << "Partition mismatch for key: " << testCase.key << ", expected: " << testCase.expectedPartition << ", got: " << choice;
}

TEST(PartitionerTest, RandomPartitioner)
{
    auto partitioner = NewRandomPartitioner("mytopic");

    // Test with 1 partition
    int32_t choice = 0;
    int result = partitioner->partition(std::make_shared<ProducerMessage>(), 1, choice);
    ASSERT_EQ(result, 0) << "Partitioning failed";
    ASSERT_EQ(choice, 0) << "Returned non-zero partition when only one available";

    // Test with 50 partitions multiple times
    for (int i = 1; i < 50; ++i)
    {
        result = partitioner->partition(std::make_shared<ProducerMessage>(), 50, choice);
        ASSERT_EQ(result, 0) << "Partitioning failed";
        ASSERT_GE(choice, 0) << "Returned partition outside of range";
        ASSERT_LT(choice, 50) << "Returned partition outside of range";
    }
}

TEST(PartitionerTest, RoundRobinPartitioner)
{
    auto partitioner = NewRoundRobinPartitioner("mytopic");

    // Test with 1 partition
    int32_t choice = 0;
    int result = partitioner->partition(std::make_shared<ProducerMessage>(), 1, choice);
    ASSERT_EQ(result, 0) << "Partitioning failed";
    ASSERT_EQ(choice, 0) << "Returned non-zero partition when only one available";

    // Test round-robin behavior with 7 partitions
    for (int32_t i = 1; i < 50; ++i)
    {
        result = partitioner->partition(std::make_shared<ProducerMessage>(), 7, choice);
        ASSERT_EQ(result, 0) << "Partitioning failed";
        ASSERT_EQ(choice, i % 7) << "Returned partition " << choice << " expecting " << (i % 7);
    }
}

TEST(PartitionerTest, HashPartitioner)
{
    auto partitioner = NewHashPartitioner("mytopic");

    // Test with 1 partition
    int32_t choice = 0;
    int result = partitioner->partition(std::make_shared<ProducerMessage>(), 1, choice);
    ASSERT_EQ(result, 0) << "Partitioning failed";
    ASSERT_EQ(choice, 0) << "Returned non-zero partition when only one available";

    // Test with 50 partitions for nil key
    for (int i = 1; i < 50; ++i)
    {
        result = partitioner->partition(std::make_shared<ProducerMessage>(), 50, choice);
        ASSERT_EQ(result, 0) << "Partitioning failed";
        ASSERT_GE(choice, 0) << "Returned partition outside of range for nil key";
        ASSERT_LT(choice, 50) << "Returned partition outside of range for nil key";
    }

    // Test consistent partitioning for messages with keys
    for (int i = 1; i < 50; ++i)
    {
        auto message = std::make_shared<ProducerMessage>();
        std::string key = "test_key_" + std::to_string(i);
        message->m_key = std::make_shared<StringEncoder>(key);
        assertPartitioningConsistent(partitioner.get(), message, 50);
    }
}

TEST(PartitionerTest, HashPartitionerConsistency)
{
    auto partitioner = NewHashPartitioner("mytopic");
    auto dynamicPartitioner = dynamic_cast<DynamicConsistencyPartitioner *>(partitioner.get());

    ASSERT_NE(dynamicPartitioner, nullptr) << "Hash partitioner does not implement DynamicConsistencyPartitioner";

    // Test that messages with keys require consistency
    auto messageWithKey = std::make_shared<ProducerMessage>();
    messageWithKey->m_key = std::make_shared<StringEncoder>("hi");
    ASSERT_TRUE(dynamicPartitioner->message_requires_consistency(messageWithKey)) << "Messages with keys should require consistency";

    // Test that messages without keys do not require consistency
    auto messageWithoutKey = std::make_shared<ProducerMessage>();
    ASSERT_FALSE(dynamicPartitioner->message_requires_consistency(messageWithoutKey)) << "Messages without keys should not require consistency";
}

TEST(PartitionerTest, ManualPartitioner)
{
    auto partitioner = std::make_shared<ManualPartitioner>();

    // Test with 1 partition
    int32_t choice = 0;
    int result = partitioner->partition(std::make_shared<ProducerMessage>(), 1, choice);
    ASSERT_EQ(result, 0) << "Partitioning failed";
    ASSERT_EQ(choice, 0) << "Returned non-zero partition when only one available";

    // Test manual partitioning
    for (int32_t i = 1; i < 50; ++i)
    {
        auto message = std::make_shared<ProducerMessage>();
        message->m_partition = i;
        result = partitioner->partition(message, 50, choice);
        ASSERT_EQ(result, 0) << "Partitioning failed";
        ASSERT_EQ(choice, i) << "Returned partition not the same as the input partition";
    }
}

TEST(PartitionerTest, ConsistentCRCHashPartitioner)
{
    int32_t numPartitions = 100;
    auto partitioner = NewConsistentCRCHashPartitioner("mytopic");

    std::vector<PartitionerTestCase> testCases = {
        {"abc123def456", 57},
        {"SheetJS", 26},
        {"9e8c7f4cf45857cfff7645d6", 24},
        {"3900446192ff85a5f67da10c", 75},
        {"0f4407b7a67d6d27de372198", 50}};

    for (const auto &tc : testCases)
    {
        partitionAndAssert(partitioner.get(), numPartitions, tc);
    }
}

TEST(PartitionerTest, RequiresConsistency)
{
    // Test that different partitioners correctly report consistency requirements

    auto randomPartitioner = NewRandomPartitioner("mytopic");
    ASSERT_FALSE(randomPartitioner->requires_consistency()) << "Random partitioner should not require consistency";

    auto roundRobinPartitioner = NewRoundRobinPartitioner("mytopic");
    ASSERT_FALSE(roundRobinPartitioner->requires_consistency()) << "Round-robin partitioner should not require consistency";

    auto hashPartitioner = NewHashPartitioner("mytopic");
    ASSERT_TRUE(hashPartitioner->requires_consistency()) << "Hash partitioner should require consistency";

    auto manualPartitioner = std::make_shared<ManualPartitioner>();
    ASSERT_TRUE(manualPartitioner->requires_consistency()) << "Manual partitioner should require consistency";

    auto consistentCRCHashPartitioner = NewConsistentCRCHashPartitioner("mytopic");
    ASSERT_TRUE(consistentCRCHashPartitioner->requires_consistency()) << "Consistent CRC hash partitioner should require consistency";
}

TEST(PartitionerTest, HashPartitionerWithSpecialKeys)
{
    // Test hash partitioner with keys that might cause edge cases
    auto partitioner = NewHashPartitioner("mytopic");

    // Test with empty string key
    auto emptyKeyMessage = std::make_shared<ProducerMessage>();
    emptyKeyMessage->m_key = std::make_shared<StringEncoder>("");
    int32_t choice = 0;
    int result = partitioner->partition(emptyKeyMessage, 50, choice);
    ASSERT_EQ(result, 0) << "Partitioning failed for empty key";
    ASSERT_GE(choice, 0) << "Returned partition outside of range for empty key";
    ASSERT_LT(choice, 50) << "Returned partition outside of range for empty key";

    // Test with long key
    std::string longKey(1000, 'a');
    auto longKeyMessage = std::make_shared<ProducerMessage>();
    longKeyMessage->m_key = std::make_shared<StringEncoder>(longKey);
    result = partitioner->partition(longKeyMessage, 50, choice);
    ASSERT_EQ(result, 0) << "Partitioning failed for long key";
    ASSERT_GE(choice, 0) << "Returned partition outside of range for long key";
    ASSERT_LT(choice, 50) << "Returned partition outside of range for long key";

    // Test with numeric key that might produce min int32
    auto minIntKeyMessage = std::make_shared<ProducerMessage>();
    minIntKeyMessage->m_key = std::make_shared<StringEncoder>("1468509572224");
    result = partitioner->partition(minIntKeyMessage, 50, choice);
    ASSERT_EQ(result, 0) << "Partitioning failed for min int32 key";
    ASSERT_GE(choice, 0) << "Returned partition outside of range for min int32 key";
    ASSERT_LT(choice, 50) << "Returned partition outside of range for min int32 key";
}
