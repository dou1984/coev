#include <gtest/gtest.h>
#include "join_group_request.h"
#include "join_group_response.h"

TEST(JoinGroupTest, RequestVersionCompatibility)
{
    JoinGroupRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 6; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(JoinGroupTest, ResponseVersionCompatibility)
{
    JoinGroupResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 6; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(JoinGroupTest, BasicFunctionality)
{
    JoinGroupRequest request;
    request.set_version(2);
    EXPECT_EQ(2, request.version());

    JoinGroupResponse response;
    response.set_version(2);
    EXPECT_EQ(2, response.version());
}