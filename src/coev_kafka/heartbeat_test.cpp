#include <gtest/gtest.h>
#include "heartbeat_request.h"
#include "heartbeat_response.h"

TEST(HeartbeatTest, RequestVersionCompatibility) {
    HeartbeatRequest request;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 4; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(HeartbeatTest, ResponseVersionCompatibility) {
    HeartbeatResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 4; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(HeartbeatTest, BasicFunctionality) {
    HeartbeatRequest request;
    request.set_version(2);
    EXPECT_EQ(2, request.version());
    
    HeartbeatResponse response;
    response.set_version(2);
    EXPECT_EQ(2, response.version());
}