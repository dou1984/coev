#include <gtest/gtest.h>
#include "acl_describe_request.h"
#include "acl_describe_response.h"

TEST(AclDescribeTest, RequestVersionCompatibility)
{
    DescribeAclsRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(AclDescribeTest, ResponseVersionCompatibility)
{
    DescribeAclsResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(AclDescribeTest, BasicFunctionality)
{
    DescribeAclsRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());

    DescribeAclsResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}