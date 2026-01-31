#include "fetch_request.h"
#include <gtest/gtest.h>
#include "real_encoder.h"
#include "real_decoder.h"

TEST(FetchRequestTest, BasicFunctionality) {
    // Test with version 0
    FetchRequest request(0);
    EXPECT_TRUE(request.is_valid_version());
    EXPECT_EQ(request.version(), 0);
    EXPECT_EQ(request.key(), apiKeyFetch);
    EXPECT_EQ(request.header_version(), 1);
}

TEST(FetchRequestTest, VersionCompatibility) {
    FetchRequest request;
    
    // Test all valid versions
    for (int16_t version = 0; version <= 11; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
    
    // Test invalid versions
    request.set_version(-1);
    EXPECT_FALSE(request.is_valid_version());
    
    request.set_version(12);
    EXPECT_FALSE(request.is_valid_version());
}

TEST(FetchRequestTest, EncodeEmptyRequest) {
    FetchRequest request(0);
    
    realEncoder encoder(1024);
    EXPECT_EQ(request.encode(encoder), 0);
}

TEST(FetchRequestTest, EncodeWithBlocks) {
    FetchRequest request(0);
    
    // Add a block
    request.add_block("test-topic", 0, 100, 1024, 0);
    
    realEncoder encoder(1024);
    EXPECT_EQ(request.encode(encoder), 0);
}

TEST(FetchRequestTest, EncodeWithVersionSpecificFields) {
    // Test with version 7 which includes session ID and epoch
    FetchRequest request(7);
    request.m_session_id = 123;
    request.m_session_epoch = 456;
    
    // Add a block
    request.add_block("test-topic", 0, 100, 1024, 0);
    
    realEncoder encoder(1024);
    EXPECT_EQ(request.encode(encoder), 0);
}