/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <gtest/gtest.h>
#include "api_versions_request.h"
#include "api_versions_response.h"

using namespace coev::kafka;

TEST(ApiVersionsTest, RequestVersionCompatibility)
{
    ApiVersionsRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 3; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(ApiVersionsTest, ResponseVersionCompatibility)
{
    ApiVersionsResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 3; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(ApiVersionsTest, BasicFunctionality)
{
    ApiVersionsRequest request;
    request.set_version(2);
    EXPECT_EQ(2, request.version());

    ApiVersionsResponse response;
    response.set_version(2);
    EXPECT_EQ(2, response.version());
}