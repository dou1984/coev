#include <gtest/gtest.h>
#include "describe_groups_request.h"

TEST(DescribeGroupsRequestTest, VersionCompatibility)
{
    DescribeGroupsRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 3; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(DescribeGroupsRequestTest, BasicFunctionality)
{
    DescribeGroupsRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());
}