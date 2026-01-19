#include <gtest/gtest.h>
#include "leave_group_request.h"

TEST(LeaveGroupRequestTest, VersionCompatibility)
{
    LeaveGroupRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 2; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(LeaveGroupRequestTest, BasicFunctionality)
{
    LeaveGroupRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());
}