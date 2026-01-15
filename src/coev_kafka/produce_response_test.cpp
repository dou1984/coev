#include <gtest/gtest.h>
#include <vector>
#include <cstdint>
#include <string>
#include <chrono>

#include "produce_response.h"
#include "real_encoder.h"
#include "real_decoder.h"

// Test data from Sarama's produce_response_test.go
const unsigned char produceResponseNoBlocksV0[] = {
    0x00, 0x00, 0x00, 0x00,
};

const unsigned char produceResponseManyBlocksV0[] = {
    0x00, 0x00, 0x00, 0x01,
    0x00, 0x03, 'f', 'o', 'o',
    0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x01, // Partition 1
    0x00, 0x02, // ErrInvalidMessage
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, // Offset 255
};

const unsigned char produceResponseManyBlocksV1[] = {
    0x00, 0x00, 0x00, 0x01,
    0x00, 0x03, 'f', 'o', 'o',
    0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x01, // Partition 1
    0x00, 0x02, // ErrInvalidMessage
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, // Offset 255
    0x00, 0x00, 0x00, 0x64, // 100 ms throttle time
};

const unsigned char produceResponseManyBlocksV2[] = {
    0x00, 0x00, 0x00, 0x01,
    0x00, 0x03, 'f', 'o', 'o',
    0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x01, // Partition 1
    0x00, 0x02, // ErrInvalidMessage
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, // Offset 255
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xE8, // Timestamp January 1st 0001 at 00:00:01,000 UTC (LogAppendTime was used)
    0x00, 0x00, 0x00, 0x64, // 100 ms throttle time
};

const unsigned char produceResponseManyBlocksV7[] = {
    0x00, 0x00, 0x00, 0x01,
    0x00, 0x03, 'f', 'o', 'o',
    0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x01, // Partition 1
    0x00, 0x02, // ErrInvalidMessage
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, // Offset 255
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xE8, // Timestamp January 1st 0001 at 00:00:01,000 UTC (LogAppendTime was used)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, // StartOffset 50
    0x00, 0x00, 0x00, 0x64, // 100 ms throttle time
};

TEST(ProduceResponseTest, DecodeNoBlocksV0) {
    realDecoder decoder;
    decoder.m_raw = std::string(reinterpret_cast<const char*>(produceResponseNoBlocksV0), sizeof(produceResponseNoBlocksV0));
    decoder.m_offset = 0;
    
    ProduceResponse response;
    int result = response.decode(decoder, 0);
    ASSERT_EQ(result, 0) << "Failed to decode produce response with no blocks";
    EXPECT_EQ(response.m_blocks.size(), 0) << "Decoding produced unexpected number of topics";
}

TEST(ProduceResponseTest, DecodeManyBlocksV0) {
    realDecoder decoder;
    decoder.m_raw = std::string(reinterpret_cast<const char*>(produceResponseManyBlocksV0), sizeof(produceResponseManyBlocksV0));
    decoder.m_offset = 0;
    
    ProduceResponse response;
    int result = response.decode(decoder, 0);
    ASSERT_EQ(result, 0) << "Failed to decode produce response with many blocks (version 0)";
    EXPECT_EQ(response.m_blocks.size(), 1) << "Decoding produced unexpected number of topics";
    EXPECT_EQ(response.m_blocks["foo"].size(), 1) << "Decoding produced unexpected number of partitions for 'foo'";
    
    auto block = response.GetBlock("foo", 1);
    ASSERT_NE(block, nullptr) << "Decoding did not produce a block for foo/1";
    EXPECT_EQ(block->m_err, KError::ErrInvalidMessage) << "Decoding failed for foo/1/Err";
    EXPECT_EQ(block->m_offset, 255) << "Decoding failed for foo/1/Offset";
}

TEST(ProduceResponseTest, DecodeManyBlocksV1) {
    realDecoder decoder;
    decoder.m_raw = std::string(reinterpret_cast<const char*>(produceResponseManyBlocksV1), sizeof(produceResponseManyBlocksV1));
    decoder.m_offset = 0;
    
    ProduceResponse response;
    int result = response.decode(decoder, 1);
    ASSERT_EQ(result, 0) << "Failed to decode produce response with many blocks (version 1)";
    
    auto block = response.GetBlock("foo", 1);
    ASSERT_NE(block, nullptr) << "Decoding did not produce a block for foo/1";
    EXPECT_EQ(block->m_err, KError::ErrInvalidMessage) << "Decoding failed for foo/1/Err";
    EXPECT_EQ(block->m_offset, 255) << "Decoding failed for foo/1/Offset";
    EXPECT_EQ(response.m_throttle_time.count(), 100) << "Failed decoding throttle time";
}

TEST(ProduceResponseTest, DecodeManyBlocksV2) {
    realDecoder decoder;
    decoder.m_raw = std::string(reinterpret_cast<const char*>(produceResponseManyBlocksV2), sizeof(produceResponseManyBlocksV2));
    decoder.m_offset = 0;
    
    ProduceResponse response;
    int result = response.decode(decoder, 2);
    ASSERT_EQ(result, 0) << "Failed to decode produce response with many blocks (version 2)";
    
    auto block = response.GetBlock("foo", 1);
    ASSERT_NE(block, nullptr) << "Decoding did not produce a block for foo/1";
    EXPECT_EQ(block->m_err, KError::ErrInvalidMessage) << "Decoding failed for foo/1/Err";
    EXPECT_EQ(block->m_offset, 255) << "Decoding failed for foo/1/Offset";
    // Verify timestamp is 1 second
    auto timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(block->m_timestamp.time_since_epoch()).count();
    EXPECT_EQ(timestampMs, 1000) << "Decoding failed for foo/1/Timestamp";
    EXPECT_EQ(response.m_throttle_time.count(), 100) << "Failed decoding throttle time";
}

TEST(ProduceResponseTest, DecodeManyBlocksV7) {
    realDecoder decoder;
    decoder.m_raw = std::string(reinterpret_cast<const char*>(produceResponseManyBlocksV7), sizeof(produceResponseManyBlocksV7));
    decoder.m_offset = 0;
    
    ProduceResponse response;
    int result = response.decode(decoder, 7);
    ASSERT_EQ(result, 0) << "Failed to decode produce response with many blocks (version 7)";
    
    auto block = response.GetBlock("foo", 1);
    ASSERT_NE(block, nullptr) << "Decoding did not produce a block for foo/1";
    EXPECT_EQ(block->m_err, KError::ErrInvalidMessage) << "Decoding failed for foo/1/Err";
    EXPECT_EQ(block->m_offset, 255) << "Decoding failed for foo/1/Offset";
    // Verify timestamp is 1 second
    auto timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(block->m_timestamp.time_since_epoch()).count();
    EXPECT_EQ(timestampMs, 1000) << "Decoding failed for foo/1/Timestamp";
    EXPECT_EQ(block->m_start_offset, 50) << "Decoding failed for foo/1/StartOffset";
    EXPECT_EQ(response.m_throttle_time.count(), 100) << "Failed decoding throttle time";
}

TEST(ProduceResponseTest, EncodeNoBlocksV0) {
    ProduceResponse response;
    response.set_version(0);
    
    realEncoder encoder;
    int result = response.encode(encoder);
    ASSERT_EQ(result, 0) << "Failed to encode produce response with no blocks";
    
    std::string expectedData(reinterpret_cast<const char*>(produceResponseNoBlocksV0), sizeof(produceResponseNoBlocksV0));
    std::string actualData(encoder.m_raw.data(), encoder.m_offset);
    EXPECT_EQ(actualData, expectedData) << "Encoding failed for no blocks response";
}

TEST(ProduceResponseTest, EncodeManyBlocksV0) {
    ProduceResponse response;
    response.set_version(0);
    
    auto block = std::make_shared<ProduceResponseBlock>();
    block->m_err = KError::ErrInvalidMessage;
    block->m_offset = 255;
    response.m_blocks["foo"][1] = block;
    
    realEncoder encoder;
    int result = response.encode(encoder);
    ASSERT_EQ(result, 0) << "Failed to encode produce response with many blocks (version 0)";
    
    std::string expectedData(reinterpret_cast<const char*>(produceResponseManyBlocksV0), sizeof(produceResponseManyBlocksV0));
    std::string actualData(encoder.m_raw.data(), encoder.m_offset);
    EXPECT_EQ(actualData, expectedData) << "Encoding failed for many blocks response (version 0)";
}

TEST(ProduceResponseTest, EncodeManyBlocksV1) {
    ProduceResponse response;
    response.set_version(1);
    
    auto block = std::make_shared<ProduceResponseBlock>();
    block->m_err = KError::ErrInvalidMessage;
    block->m_offset = 255;
    response.m_blocks["foo"][1] = block;
    response.m_throttle_time = std::chrono::milliseconds(100);
    
    realEncoder encoder;
    int result = response.encode(encoder);
    ASSERT_EQ(result, 0) << "Failed to encode produce response with many blocks (version 1)";
    
    std::string expectedData(reinterpret_cast<const char*>(produceResponseManyBlocksV1), sizeof(produceResponseManyBlocksV1));
    std::string actualData(encoder.m_raw.data(), encoder.m_offset);
    EXPECT_EQ(actualData, expectedData) << "Encoding failed for many blocks response (version 1)";
}

TEST(ProduceResponseTest, EncodeManyBlocksV2) {
    ProduceResponse response;
    response.set_version(2);
    
    auto block = std::make_shared<ProduceResponseBlock>();
    block->m_err = KError::ErrInvalidMessage;
    block->m_offset = 255;
    // Set timestamp to 1 second since epoch
    block->m_timestamp = std::chrono::system_clock::from_time_t(1);
    response.m_blocks["foo"][1] = block;
    response.m_throttle_time = std::chrono::milliseconds(100);
    
    realEncoder encoder;
    int result = response.encode(encoder);
    ASSERT_EQ(result, 0) << "Failed to encode produce response with many blocks (version 2)";
    
    std::string expectedData(reinterpret_cast<const char*>(produceResponseManyBlocksV2), sizeof(produceResponseManyBlocksV2));
    std::string actualData(encoder.m_raw.data(), encoder.m_offset);
    EXPECT_EQ(actualData, expectedData) << "Encoding failed for many blocks response (version 2)";
}

TEST(ProduceResponseTest, EncodeManyBlocksV7) {
    ProduceResponse response;
    response.set_version(7);
    
    auto block = std::make_shared<ProduceResponseBlock>();
    block->m_err = KError::ErrInvalidMessage;
    block->m_offset = 255;
    // Set timestamp to 1 second since epoch
    block->m_timestamp = std::chrono::system_clock::from_time_t(1);
    block->m_start_offset = 50;
    response.m_blocks["foo"][1] = block;
    response.m_throttle_time = std::chrono::milliseconds(100);
    
    realEncoder encoder;
    int result = response.encode(encoder);
    ASSERT_EQ(result, 0) << "Failed to encode produce response with many blocks (version 7)";
    
    std::string expectedData(reinterpret_cast<const char*>(produceResponseManyBlocksV7), sizeof(produceResponseManyBlocksV7));
    std::string actualData(encoder.m_raw.data(), encoder.m_offset);
    EXPECT_EQ(actualData, expectedData) << "Encoding failed for many blocks response (version 7)";
}

TEST(ProduceResponseTest, VersionProperties) {
    ProduceResponse response;
    
    // Test version 0
    response.set_version(0);
    EXPECT_EQ(response.version(), 0) << "Version not properly set";
    EXPECT_EQ(response.header_version(), 0) << "Header version mismatch";
    EXPECT_EQ(response.key(), 0) << "Key mismatch";
    EXPECT_TRUE(response.is_valid_version()) << "Version 0 should be valid";
    EXPECT_EQ(response.required_version(), V0_8_2_0) << "Required version mismatch for version 0";
    
    // Test version 7
    response.set_version(7);
    EXPECT_EQ(response.version(), 7) << "Version not properly set";
    EXPECT_TRUE(response.is_valid_version()) << "Version 7 should be valid";
    EXPECT_EQ(response.required_version(), V2_1_0_0) << "Required version mismatch for version 7";
    
    // Test invalid version
    response.set_version(8);
    EXPECT_FALSE(response.is_valid_version()) << "Version 8 should be invalid";
}

TEST(ProduceResponseTest, GetBlock) {
    ProduceResponse response;
    
    // Test getting non-existent block
    auto block = response.GetBlock("nonexistent", 0);
    EXPECT_EQ(block, nullptr) << "GetBlock should return nullptr for non-existent topic";
    
    // Add a block and test getting it
    auto blockPtr = std::make_shared<ProduceResponseBlock>();
    blockPtr->m_err = KError::ErrNoError;
    blockPtr->m_offset = 123;
    response.m_blocks["test"][5] = blockPtr;
    
    auto retrievedBlock = response.GetBlock("test", 5);
    EXPECT_EQ(retrievedBlock, blockPtr) << "GetBlock should return the correct block";
    
    auto nonExistentPartitionBlock = response.GetBlock("test", 6);
    EXPECT_EQ(nonExistentPartitionBlock, nullptr) << "GetBlock should return nullptr for non-existent partition";
}

TEST(ProduceResponseTest, AddTopicPartition) {
    ProduceResponse response;
    
    // Test AddTopicPartition
    response.set_version(0);
    response.AddTopicPartition("new_topic", 3, KError::ErrInvalidMessage);
    
    EXPECT_EQ(response.m_blocks.size(), 1) << "AddTopicPartition should add a new topic";
    EXPECT_EQ(response.m_blocks["new_topic"].size(), 1) << "AddTopicPartition should add a new partition";
    
    auto block = response.GetBlock("new_topic", 3);
    ASSERT_NE(block, nullptr) << "AddTopicPartition should create a block";
    EXPECT_EQ(block->m_err, KError::ErrInvalidMessage) << "AddTopicPartition should set the correct error";
    
    // Test AddTopicPartition with version >= 2 (should set timestamp)
    ProduceResponse responseV2;
    responseV2.set_version(2);
    responseV2.AddTopicPartition("new_topic", 3, KError::ErrNoError);
    
    auto blockV2 = responseV2.GetBlock("new_topic", 3);
    ASSERT_NE(blockV2, nullptr) << "AddTopicPartition should create a block for version 2";
    // Timestamp should be set to current time, just check it's not zero
    EXPECT_GT(blockV2->m_timestamp.time_since_epoch().count(), 0) << "AddTopicPartition should set timestamp for version >= 2";
}
