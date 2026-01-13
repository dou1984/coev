#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <memory>

#include "metadata_response.h"
#include "real_encoder.h"
#include "real_decoder.h"

// Simplified test to focus on basic functionality
TEST(MetadataResponseTest, BasicEncodingDecoding)
{
    // Create a simple metadata response
    MetadataResponse response;
    response.set_version(0);
    
    // Add a broker
    response.AddBroker("localhost:9092", 0);
    
    // Add a topic with one partition
    auto topic = response.AddTopic("topic1", 0);
    ASSERT_NE(topic, nullptr);
    
    // Test encoding
    realEncoder encoder;
    int encodeResult = response.encode(encoder);
    ASSERT_EQ(encodeResult, 0) << "Failed to encode response";
    
    const auto& encoded = encoder.m_raw;
    ASSERT_FALSE(encoded.empty()) << "Encoded data is empty";
    
    // Test decoding
    realDecoder decoder;
    decoder.m_raw = encoded;
    decoder.m_offset = 0;
    
    MetadataResponse decoded;
    decoded.set_version(0);
    int decodeResult = decoded.decode(decoder, 0);
    ASSERT_EQ(decodeResult, 0) << "Failed to decode response";
    
    // Verify decoded values
    ASSERT_EQ(decoded.Brokers.size(), 1) << "Brokers count mismatch";
    ASSERT_EQ(decoded.Topics.size(), 1) << "Topics count mismatch";
}

// Test version properties
TEST(MetadataResponseTest, VersionProperties)
{
    MetadataResponse response;
    
    // Check for non-flexible version
    response.set_version(8);
    EXPECT_FALSE(response.is_flexible());
    
    // Check for flexible version
    response.set_version(9);
    EXPECT_TRUE(response.is_flexible());
    
    // Check key and version methods
    response.set_version(5);
    EXPECT_EQ(response.key(), 3) << "MetadataResponse key mismatch";
    EXPECT_EQ(response.version(), 5) << "Version mismatch";
    EXPECT_EQ(response.header_version(), 0) << "Header version mismatch";
    EXPECT_TRUE(response.is_valid_version()) << "Version should be valid";
}

// Test broker addition
TEST(MetadataResponseTest, BrokerAddition)
{
    MetadataResponse response;
    response.set_version(0);
    
    // Add brokers
    response.AddBroker("broker1:9092", 1);
    response.AddBroker("broker2:9092", 2);
    
    ASSERT_EQ(response.Brokers.size(), 2) << "Brokers count mismatch";
}

// Test topic and partition addition
TEST(MetadataResponseTest, TopicPartitionAddition)
{
    MetadataResponse response;
    response.set_version(0);
    
    // Add a topic with partitions
    std::vector<int32_t> replicas = {1, 2, 3};
    std::vector<int32_t> isr = {1, 2};
    std::vector<int32_t> offline = {3};
    
    int result = response.AddTopicPartition(
        "test-topic", 0, 1, replicas, isr, offline, 0
    );
    
    ASSERT_EQ(result, 0) << "Failed to add topic partition";
    ASSERT_EQ(response.Topics.size(), 1) << "Topics count mismatch";
    ASSERT_EQ(response.Topics[0]->Partitions.size(), 1) << "Partitions count mismatch";
}

// Test flexible version handling
TEST(MetadataResponseTest, FlexibleVersionHandling)
{
    MetadataResponse response;
    
    // Check flexible version check
    EXPECT_TRUE(response.is_flexible_version(9));
    EXPECT_FALSE(response.is_flexible_version(8));
}

// Test required version calculation
TEST(MetadataResponseTest, RequiredVersion)
{
    MetadataResponse response;
    
    // Set different versions and check required Kafka version
    response.set_version(0);
    EXPECT_EQ(response.required_version(), V0_8_2_0);
    
    response.set_version(5);
    EXPECT_EQ(response.required_version(), V1_0_0_0);
    
    response.set_version(10);
    EXPECT_EQ(response.required_version(), V2_8_0_0);
}


