/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <gtest/gtest.h>
#include "create_topics_response.h"

using namespace coev::kafka;

TEST(CreateTopicsResponseTest, VersionCompatibility)
{
    CreateTopicsResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 6; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(CreateTopicsResponseTest, BasicFunctionality)
{
    CreateTopicsResponse response;
    response.set_version(3);
    EXPECT_EQ(3, response.version());
}