#include <gtest/gtest.h>
#include "describe_configs_request.h"
#include "describe_configs_response.h"
#include "api_versions.h"

TEST(DescribeConfigsTest, RequestVersionCompatibility)
{
    DescribeConfigsRequest request;
    for (int16_t version = 0; version <= 4; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(DescribeConfigsTest, ResponseVersionCompatibility)
{
    DescribeConfigsResponse response;
    for (int16_t version = 0; version <= 4; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(DescribeConfigsTest, BasicFunctionality)
{
    DescribeConfigsRequest request;
    request.set_version(1);
    EXPECT_EQ(request.version(), 1);
    EXPECT_EQ(request.key(), apiKeyDescribeConfigs);

    DescribeConfigsResponse response;
    response.set_version(1);
    EXPECT_EQ(response.version(), 1);
    EXPECT_EQ(response.key(), apiKeyDescribeConfigs);
}