#include "find_coordinator_request.h"
#include <gtest/gtest.h>
#include "real_encoder.h"
#include "packet_decoder.h"

TEST(FindCoordinatorRequestTest, BasicFunctionality) {
    // Test with version 0
    FindCoordinatorRequest request;
    request.set_version(0);
    EXPECT_TRUE(request.is_valid_version());
    EXPECT_EQ(request.version(), 0);
    EXPECT_EQ(request.key(), apiKeyFindCoordinator);
    EXPECT_EQ(request.header_version(), 1);
}

TEST(FindCoordinatorRequestTest, VersionCompatibility) {
    FindCoordinatorRequest request;
    
    // Test all valid versions (0-3)
    for (int16_t version = 0; version <= 3; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
    
    // Test invalid versions
    request.set_version(-1);
    EXPECT_FALSE(request.is_valid_version());
    
    request.set_version(4);
    EXPECT_FALSE(request.is_valid_version());
}

TEST(FindCoordinatorRequestTest, EncodeWithVersion0) {
    FindCoordinatorRequest request;
    request.set_version(0);
    request.m_coordinator_key = "test-group";
    
    real_encoder encoder(1024);
    EXPECT_EQ(request.encode(encoder), 0);
}

TEST(FindCoordinatorRequestTest, EncodeWithVersion1) {
    FindCoordinatorRequest request;
    request.set_version(1);
    request.m_coordinator_key = "test-group";
    request.m_coordinator_type = CoordinatorGroup;
    
    real_encoder encoder(1024);
    EXPECT_EQ(request.encode(encoder), 0);
}

TEST(FindCoordinatorRequestTest, EncodeWithVersion2) {
    FindCoordinatorRequest request;
    request.set_version(2);
    request.m_coordinator_key = "test-group";
    request.m_coordinator_type = CoordinatorGroup;
    
    real_encoder encoder(1024);
    EXPECT_EQ(request.encode(encoder), 0);
}