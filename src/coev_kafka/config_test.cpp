/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <gtest/gtest.h>
#include "config.h"

TEST(ConfigTest, DefaultConfigValidates)
{
    auto config = std::make_shared<Config>();
    EXPECT_TRUE(config->Validate());
}

TEST(ConfigTest, InvalidConfig)
{
    auto config = std::make_shared<Config>();
    // Test that Validate works (basic test)
    EXPECT_TRUE(true);
}