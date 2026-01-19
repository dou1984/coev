#include <gtest/gtest.h>
#include "describe_log_dirs_request.h"
#include "describe_log_dirs_response.h"

TEST(DescribeLogDirsTest, RequestVersionCompatibility)
{
    DescribeLogDirsRequest request;
    for (int16_t version = 0; version <= 1; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(DescribeLogDirsTest, ResponseVersionCompatibility)
{
    DescribeLogDirsResponse response;
    for (int16_t version = 0; version <= 1; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(DescribeLogDirsTest, BasicFunctionality)
{
    DescribeLogDirsRequest request;
    request.set_version(1);
    EXPECT_EQ(request.version(), 1);
    EXPECT_EQ(request.key(), 35);

    DescribeLogDirsResponse response;
    response.set_version(1);
    EXPECT_EQ(response.version(), 1);
    EXPECT_EQ(response.key(), 35);
}