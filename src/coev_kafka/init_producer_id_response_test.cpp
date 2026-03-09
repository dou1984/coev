/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <gtest/gtest.h>
#include "init_producer_id_response.h"

using namespace coev::kafka;
TEST(InitProducerIdResponseTest, VersionCompatibility)
{
    InitProducerIDResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 2; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(InitProducerIdResponseTest, BasicFunctionality)
{
    InitProducerIDResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}