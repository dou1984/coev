#include <gtest/gtest.h>
#include "acl_delete_request.h"
#include "acl_delete_response.h"

TEST(AclDeleteTest, RequestVersionCompatibility)
{
    DeleteAclsRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(AclDeleteTest, ResponseVersionCompatibility)
{
    DeleteAclsResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(AclDeleteTest, BasicFunctionality)
{
    DeleteAclsRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());

    DeleteAclsResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}