#include <gtest/gtest.h>
#include "join_group_request.h"

TEST(JoinGroupRequestTest, VersionCompatibility) {
    JoinGroupRequest request;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 6; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(JoinGroupRequestTest, BasicFunctionality) {
    JoinGroupRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());
}