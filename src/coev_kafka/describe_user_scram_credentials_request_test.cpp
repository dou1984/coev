#include <gtest/gtest.h>
#include "describe_user_scram_credentials_request.h"

TEST(DescribeUserScramCredentialsRequestTest, VersionCompatibility)
{
    DescribeUserScramCredentialsRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(DescribeUserScramCredentialsRequestTest, BasicFunctionality)
{
    DescribeUserScramCredentialsRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());
}