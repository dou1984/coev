#include <gtest/gtest.h>
#include "find_coordinator_response.h"

TEST(FindCoordinatorResponseTest, VersionCompatibility) {
    FindCoordinatorResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 3; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(FindCoordinatorResponseTest, BasicFunctionality) {
    FindCoordinatorResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}