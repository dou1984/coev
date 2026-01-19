#include <gtest/gtest.h>
#include "describe_groups_request.h"
#include "describe_groups_response.h"

TEST(DescribeGroupsTest, RequestVersionCompatibility)
{
    DescribeGroupsRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 3; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(DescribeGroupsTest, ResponseVersionCompatibility)
{
    DescribeGroupsResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 3; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(DescribeGroupsTest, BasicFunctionality)
{
    DescribeGroupsRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());

    DescribeGroupsResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}