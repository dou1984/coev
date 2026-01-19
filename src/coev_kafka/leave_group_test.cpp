#include <gtest/gtest.h>
#include "leave_group_request.h"
#include "leave_group_response.h"

TEST(LeaveGroupTest, RequestVersionCompatibility)
{
    LeaveGroupRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 4; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(LeaveGroupTest, ResponseVersionCompatibility)
{
    LeaveGroupResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 4; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(LeaveGroupTest, BasicFunctionality)
{
    LeaveGroupRequest request;
    request.set_version(2);
    EXPECT_EQ(2, request.version());

    LeaveGroupResponse response;
    response.set_version(2);
    EXPECT_EQ(2, response.version());
}