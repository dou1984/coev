#include <gtest/gtest.h>
#include "heartbeat_request.h"

TEST(HeartbeatRequestTest, VersionCompatibility) {
    HeartbeatRequest request;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 2; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(HeartbeatRequestTest, BasicFunctionality) {
    HeartbeatRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());
}