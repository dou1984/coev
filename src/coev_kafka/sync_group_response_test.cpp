#include <gtest/gtest.h>
#include "sync_group_response.h"

TEST(SyncGroupResponseTest, VersionCompatibility) {
    SyncGroupResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 5; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(SyncGroupResponseTest, BasicFunctionality) {
    SyncGroupResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}