/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <gtest/gtest.h>
#include "produce_request.h"

TEST(ProduceRequestTest, VersionCompatibility)
{
    ProduceRequest request;

    // Test all supported versions
    for (int16_t version = 0; version <= 7; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(ProduceRequestTest, BasicFunctionality)
{
    ProduceRequest request;
    request.set_version(5);
    EXPECT_EQ(5, request.version());
}