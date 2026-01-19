#include "incremental_alter_configs_response.h"
#include <gtest/gtest.h>
#include "real_encoder.h"
#include "packet_decoder.h"

TEST(IncrementalAlterConfigsResponseTest, BasicFunctionality)
{
    // Test with version 0
    IncrementalAlterConfigsResponse response;
    response.set_version(0);
    EXPECT_TRUE(response.is_valid_version());
    EXPECT_EQ(response.version(), 0);
    EXPECT_EQ(response.key(), apiKeyIncrementalAlterConfigs);
}

TEST(IncrementalAlterConfigsResponseTest, VersionCompatibility)
{
    IncrementalAlterConfigsResponse response;

    // Test all valid versions (0-1)
    for (int16_t version = 0; version <= 1; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }

    // Test invalid versions
    response.set_version(-1);
    EXPECT_FALSE(response.is_valid_version());

    response.set_version(2);
    EXPECT_FALSE(response.is_valid_version());
}

TEST(IncrementalAlterConfigsResponseTest, EncodeEmptyResponse)
{
    IncrementalAlterConfigsResponse response;
    response.set_version(0);

    realEncoder encoder(1024);
    EXPECT_EQ(response.encode(encoder), 0);
}

TEST(IncrementalAlterConfigsResponseTest, EncodeWithVersion0)
{
    IncrementalAlterConfigsResponse response;
    response.set_version(0);
    response.m_throttle_time = std::chrono::milliseconds(100);

    realEncoder encoder(1024);
    EXPECT_EQ(response.encode(encoder), 0);
}

TEST(IncrementalAlterConfigsResponseTest, EncodeWithVersion1)
{
    IncrementalAlterConfigsResponse response;
    response.set_version(1);
    response.m_throttle_time = std::chrono::milliseconds(100);

    realEncoder encoder(1024);
    EXPECT_EQ(response.encode(encoder), 0);
}