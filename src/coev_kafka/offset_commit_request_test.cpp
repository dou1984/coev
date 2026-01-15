#include <gtest/gtest.h>
#include "offset_commit_request.h"
#include "real_encoder.h"
#include "real_decoder.h"

TEST(OffsetCommitRequestTest, V0) {
    auto request = std::make_shared<OffsetCommitRequest>(0);
    request->m_consumer_group = "foobar";
    
    // Test no blocks v0
    std::vector<uint8_t> expected_no_blocks_v0 = {
        0x00, 0x06, 'f', 'o', 'o', 'b', 'a', 'r',
        0x00, 0x00, 0x00, 0x00
    };
    
    realEncoder pe(1024);
    request->encode(pe);
    EXPECT_EQ(pe.m_raw.substr(0, pe.m_offset), std::string(expected_no_blocks_v0.begin(), expected_no_blocks_v0.end()));
    
    // Test one block v0
    request->AddBlock("topic", 0x5221, 0xDEADBEEF, 0, "metadata");
    
    std::vector<uint8_t> expected_one_block_v0 = {
        0x00, 0x06, 'f', 'o', 'o', 'b', 'a', 'r',
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x05, 't', 'o', 'p', 'i', 'c',
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x52, 0x21,
        0x00, 0x00, 0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xEF,
        0x00, 0x08, 'm', 'e', 't', 'a', 'd', 'a', 't', 'a'
    };
    
    realEncoder pe_one(1024);
    request->encode(pe_one);
    EXPECT_EQ(pe_one.m_raw.substr(0, pe_one.m_offset), std::string(expected_one_block_v0.begin(), expected_one_block_v0.end()));
}

TEST(OffsetCommitRequestTest, V1) {
    auto request = std::make_shared<OffsetCommitRequest>(1);
    request->m_consumer_group = "foobar";
    request->m_consumer_id = "cons";
    request->m_consumer_group_generation = 0x1122;
    
    // Test no blocks v1
    std::vector<uint8_t> expected_no_blocks_v1 = {
        0x00, 0x06, 'f', 'o', 'o', 'b', 'a', 'r',
        0x00, 0x00, 0x11, 0x22,
        0x00, 0x04, 'c', 'o', 'n', 's',
        0x00, 0x00, 0x00, 0x00
    };
    
    realEncoder pe(1024);
    request->encode(pe);
    EXPECT_EQ(pe.m_raw.substr(0, pe.m_offset), std::string(expected_no_blocks_v1.begin(), expected_no_blocks_v1.end()));
    
    // Test one block v1
    request->AddBlock("topic", 0x5221, 0xDEADBEEF, -1, "metadata");
    
    std::vector<uint8_t> expected_one_block_v1 = {
        0x00, 0x06, 'f', 'o', 'o', 'b', 'a', 'r',
        0x00, 0x00, 0x11, 0x22,
        0x00, 0x04, 'c', 'o', 'n', 's',
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x05, 't', 'o', 'p', 'i', 'c',
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x52, 0x21,
        0x00, 0x00, 0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xEF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x08, 'm', 'e', 't', 'a', 'd', 'a', 't', 'a'
    };
    
    realEncoder pe_one(1024);
    request->encode(pe_one);
    EXPECT_EQ(pe_one.m_raw.substr(0, pe_one.m_offset), std::string(expected_one_block_v1.begin(), expected_one_block_v1.end()));
}

TEST(OffsetCommitRequestTest, V2ToV4) {
    for (int16_t version = 2; version <= 4; version++) {
        auto request = std::make_shared<OffsetCommitRequest>(version);
        request->m_consumer_group = "foobar";
        request->m_consumer_id = "cons";
        request->m_consumer_group_generation = 0x1122;
        request->m_retention_time = 0x4433;
        
        // Test no blocks v2-v4
        std::vector<uint8_t> expected_no_blocks_v2 = {
            0x00, 0x06, 'f', 'o', 'o', 'b', 'a', 'r',
            0x00, 0x00, 0x11, 0x22,
            0x00, 0x04, 'c', 'o', 'n', 's',
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x33,
            0x00, 0x00, 0x00, 0x00
        };
        
        realEncoder pe(1024);
        request->encode(pe);
        EXPECT_EQ(pe.m_raw.substr(0, pe.m_offset), std::string(expected_no_blocks_v2.begin(), expected_no_blocks_v2.end()));
        
        // Test one block v2-v4
        request->AddBlock("topic", 0x5221, 0xDEADBEEF, 0, "metadata");
        
        std::vector<uint8_t> expected_one_block_v2 = {
            0x00, 0x06, 'f', 'o', 'o', 'b', 'a', 'r',
            0x00, 0x00, 0x11, 0x22,
            0x00, 0x04, 'c', 'o', 'n', 's',
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x33,
            0x00, 0x00, 0x00, 0x01,
            0x00, 0x05, 't', 'o', 'p', 'i', 'c',
            0x00, 0x00, 0x00, 0x01,
            0x00, 0x00, 0x52, 0x21,
            0x00, 0x00, 0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xEF,
            0x00, 0x08, 'm', 'e', 't', 'a', 'd', 'a', 't', 'a'
        };
        
        realEncoder pe_one(1024);
        request->encode(pe_one);
        EXPECT_EQ(pe_one.m_raw.substr(0, pe_one.m_offset), std::string(expected_one_block_v2.begin(), expected_one_block_v2.end()));
    }
}

TEST(OffsetCommitRequestTest, V5AndPlus) {
    // Test V5
    auto request_v5 = std::make_shared<OffsetCommitRequest>(5);
    request_v5->m_consumer_group = "foo";
    request_v5->m_consumer_group_generation = 1;
    request_v5->m_consumer_id = "mid";
    request_v5->AddBlock("topic", 1, 2, 0, "meta");
    
    std::vector<uint8_t> expected_v5 = {
        0, 3, 'f', 'o', 'o', // GroupId
        0x00, 0x00, 0x00, 0x01, // GenerationId
        0, 3, 'm', 'i', 'd', // MemberId
        0, 0, 0, 1, // One Topic
        0, 5, 't', 'o', 'p', 'i', 'c', // Name
        0, 0, 0, 1, // One Partition
        0, 0, 0, 1, // PartitionIndex
        0, 0, 0, 0, 0, 0, 0, 2, // CommittedOffset
        0, 4, 'm', 'e', 't', 'a', // CommittedMetadata
    };
    
    realEncoder pe_v5(1024);
    request_v5->encode(pe_v5);
    EXPECT_EQ(pe_v5.m_raw.substr(0, pe_v5.m_offset), std::string(expected_v5.begin(), expected_v5.end()));
    
    // Test V6
    auto request_v6 = std::make_shared<OffsetCommitRequest>(6);
    request_v6->m_consumer_group = "foo";
    request_v6->m_consumer_group_generation = 1;
    request_v6->m_consumer_id = "mid";
    request_v6->AddBlockWithLeaderEpoch("topic", 1, 2, 3, 0, "meta");
    
    std::vector<uint8_t> expected_v6 = {
        0, 3, 'f', 'o', 'o', // GroupId
        0x00, 0x00, 0x00, 0x01, // GenerationId
        0, 3, 'm', 'i', 'd', // MemberId
        0, 0, 0, 1, // One Topic
        0, 5, 't', 'o', 'p', 'i', 'c', // Name
        0, 0, 0, 1, // One Partition
        0, 0, 0, 1, // PartitionIndex
        0, 0, 0, 0, 0, 0, 0, 2, // CommittedOffset
        0, 0, 0, 3, // CommittedEpoch
        0, 4, 'm', 'e', 't', 'a', // CommittedMetadata
    };
    
    realEncoder pe_v6(1024);
    request_v6->encode(pe_v6);
    EXPECT_EQ(pe_v6.m_raw.substr(0, pe_v6.m_offset), std::string(expected_v6.begin(), expected_v6.end()));
    
    // Test V7
    auto request_v7 = std::make_shared<OffsetCommitRequest>(7);
    request_v7->m_consumer_group = "foo";
    request_v7->m_consumer_group_generation = 1;
    request_v7->m_consumer_id = "mid";
    request_v7->m_group_instance_id = "gid";
    request_v7->AddBlockWithLeaderEpoch("topic", 1, 2, 3, 0, "meta");
    
    std::vector<uint8_t> expected_v7 = {
        0, 3, 'f', 'o', 'o', // GroupId
        0x00, 0x00, 0x00, 0x01, // GenerationId
        0, 3, 'm', 'i', 'd', // MemberId
        0, 3, 'g', 'i', 'd', // MemberId
        0, 0, 0, 1, // One Topic
        0, 5, 't', 'o', 'p', 'i', 'c', // Name
        0, 0, 0, 1, // One Partition
        0, 0, 0, 1, // PartitionIndex
        0, 0, 0, 0, 0, 0, 0, 2, // CommittedOffset
        0, 0, 0, 3, // CommittedEpoch
        0, 4, 'm', 'e', 't', 'a', // CommittedMetadata
    };
    
    realEncoder pe_v7(1024);
    request_v7->encode(pe_v7);
    EXPECT_EQ(pe_v7.m_raw.substr(0, pe_v7.m_offset), std::string(expected_v7.begin(), expected_v7.end()));
}

TEST(OffsetCommitRequestTest, Decode) {
    // Test decoding V5 request
    std::vector<uint8_t> v5_bytes = {
        0, 3, 'f', 'o', 'o', // GroupId
        0x00, 0x00, 0x00, 0x01, // GenerationId
        0, 3, 'm', 'i', 'd', // MemberId
        0, 0, 0, 1, // One Topic
        0, 5, 't', 'o', 'p', 'i', 'c', // Name
        0, 0, 0, 1, // One Partition
        0, 0, 0, 1, // PartitionIndex
        0, 0, 0, 0, 0, 0, 0, 2, // CommittedOffset
        0, 4, 'm', 'e', 't', 'a', // CommittedMetadata
    };
    
    auto request = std::make_shared<OffsetCommitRequest>(5);
    realDecoder pd_v5;
    pd_v5.m_raw = std::string(v5_bytes.begin(), v5_bytes.end());
    EXPECT_EQ(request->decode(pd_v5, 5), 0);
    EXPECT_EQ(request->m_consumer_group, "foo");
    EXPECT_EQ(request->m_consumer_group_generation, 1);
    EXPECT_EQ(request->m_consumer_id, "mid");
    EXPECT_EQ(request->m_group_instance_id, "");
    
    // Test decoding V7 request
    std::vector<uint8_t> v7_bytes = {
        0, 3, 'f', 'o', 'o', // GroupId
        0x00, 0x00, 0x00, 0x01, // GenerationId
        0, 3, 'm', 'i', 'd', // MemberId
        0, 3, 'g', 'i', 'd', // MemberId
        0, 0, 0, 1, // One Topic
        0, 5, 't', 'o', 'p', 'i', 'c', // Name
        0, 0, 0, 1, // One Partition
        0, 0, 0, 1, // PartitionIndex
        0, 0, 0, 0, 0, 0, 0, 2, // CommittedOffset
        0, 0, 0, 3, // CommittedEpoch
        0, 4, 'm', 'e', 't', 'a', // CommittedMetadata
    };
    
    auto request_v7 = std::make_shared<OffsetCommitRequest>(7);
    realDecoder pd_v7;
    pd_v7.m_raw = std::string(v7_bytes.begin(), v7_bytes.end());
    EXPECT_EQ(request_v7->decode(pd_v7, 7), 0);
    EXPECT_EQ(request_v7->m_consumer_group, "foo");
    EXPECT_EQ(request_v7->m_consumer_group_generation, 1);
    EXPECT_EQ(request_v7->m_consumer_id, "mid");
    EXPECT_EQ(request_v7->m_group_instance_id, "gid");
}