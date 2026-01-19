#include <gtest/gtest.h>
#include "leave_group_response.h"

TEST(LeaveGroupResponseTest, VersionCompatibility)
{
    LeaveGroupResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 2; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(LeaveGroupResponseTest, BasicFunctionality)
{
    LeaveGroupResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}