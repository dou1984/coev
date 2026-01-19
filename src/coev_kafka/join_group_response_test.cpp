#include <gtest/gtest.h>
#include "join_group_response.h"

TEST(JoinGroupResponseTest, VersionCompatibility)
{
    JoinGroupResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 6; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(JoinGroupResponseTest, BasicFunctionality)
{
    JoinGroupResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}