#include <gtest/gtest.h>
#include "heartbeat_response.h"

TEST(HeartbeatResponseTest, VersionCompatibility) {
    HeartbeatResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 2; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(HeartbeatResponseTest, BasicFunctionality) {
    HeartbeatResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}