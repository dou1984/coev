/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <gtest/gtest.h>
#include "metadata_request.h"

TEST(MetadataRequestTest, VersionCompatibility)
{
    MetadataRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 10; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(MetadataRequestTest, BasicFunctionality)
{
    MetadataRequest request;
    request.set_version(7);
    EXPECT_EQ(7, request.version());
}