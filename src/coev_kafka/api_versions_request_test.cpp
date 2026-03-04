/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <gtest/gtest.h>
#include "api_versions_request.h"

TEST(ApiVersionsRequestTest, VersionCompatibility)
{
    ApiVersionsRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 3; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(ApiVersionsRequestTest, BasicFunctionality)
{
    ApiVersionsRequest request;
    request.set_version(1);
    EXPECT_EQ(1, request.version());
}