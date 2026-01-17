#include <gtest/gtest.h>
#include "list_groups_response.h"

TEST(ListGroupsResponseTest, VersionCompatibility) {
    ListGroupsResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 4; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(ListGroupsResponseTest, BasicFunctionality) {
    ListGroupsResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}