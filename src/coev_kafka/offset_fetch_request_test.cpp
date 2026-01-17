#include "offset_fetch_request.h"
#include <gtest/gtest.h>
#include "real_encoder.h"
#include "real_decoder.h"

TEST(OffsetFetchRequestTest, BasicFunctionality) {
    // Test with version 0
    OffsetFetchRequest request;
    request.set_version(0);
    EXPECT_TRUE(request.is_valid_version());
    EXPECT_EQ(request.version(), 0);
    EXPECT_EQ(request.key(), apiKeyOffsetFetch);
}

TEST(OffsetFetchRequestTest, VersionCompatibility) {
    OffsetFetchRequest request;
    
    // Test all valid versions (0-7)
    for (int16_t version = 0; version <= 7; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
    
    // Test invalid versions
    request.set_version(-1);
    EXPECT_FALSE(request.is_valid_version());
    
    request.set_version(8);
    EXPECT_FALSE(request.is_valid_version());
}

TEST(OffsetFetchRequestTest, EncodeEmptyRequest) {
    OffsetFetchRequest request;
    request.set_version(0);
    request.m_consumer_group = "test-group";
    
    realEncoder encoder(1024);
    EXPECT_EQ(request.encode(encoder), 0);
}

TEST(OffsetFetchRequestTest, EncodeWithPartitions) {
    OffsetFetchRequest request;
    request.set_version(0);
    request.m_consumer_group = "test-group";
    
    // Add partitions
    request.AddPartition("test-topic", 0);
    request.AddPartition("test-topic", 1);
    
    realEncoder encoder(1024);
    EXPECT_EQ(request.encode(encoder), 0);
}

TEST(OffsetFetchRequestTest, EncodeWithRequireStable) {
    // Test with version 7 which includes require_stable flag
    OffsetFetchRequest request;
    request.set_version(7);
    request.m_consumer_group = "test-group";
    request.m_require_stable = true;
    
    realEncoder encoder(1024);
    EXPECT_EQ(request.encode(encoder), 0);
}